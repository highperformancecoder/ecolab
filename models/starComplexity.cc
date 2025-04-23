#include "starComplexity.h"
#include "starComplexity.cd"
#include "netcomplexity.h"
#include "pythonBuffer.h"
#include "ecolab_epilogue.h"

#include <algorithm>
#include <assert.h>

StarComplexityGen starC;
CLASSDESC_ADD_GLOBAL(starC);
CLASSDESC_PYTHON_MODULE(starComplexity);

using namespace std;
using ecolab::NautyRep;

// generate a an edge between node i and j, non-directional
linkRep edge(unsigned i, unsigned j)
{
  if (i==j) return 0; // no self-edges
  if (i<j) swap(i,j);
  return linkRep(1)<<(i*(i-1)/2+j);
}

void StarComplexityGen::generateElementaryStars(unsigned nodes)
{
  for (unsigned i=0; i<nodes; ++i)
    {
      linkRep star=0;
      for (unsigned j=0; j<nodes; ++j)
        star|=edge(i,j);
      elemStars.push_back(star);
    }
}

constexpr int setUnion=-2, setIntersection=-1;
using Recipe=vector<int,Alloc<int>>;

inline unsigned countStars(const Recipe& recipe)
{
  return accumulate(recipe.begin(), recipe.end(), 0U,
                    [](unsigned a, int i) {return a+(i>=0);});
}

// structure holding position vector of stars within a recipe
struct Pos: public vector<int,Alloc<int>>
{
  Pos(int numStars) {
    assert(numStars>1);
    for (int i=0; i<numStars; ++i) push_back(i);
  }
  bool next() {return next(size()-1);}
  bool next(int starIdx) {
    if (starIdx==1) return false;
    auto& p=operator[](starIdx);
    if (++p>2*starIdx-1) // exhausted positions, move next star down
      {
        p=operator[](starIdx-1)+1;
        return next(starIdx-1);
      }
    return true;
  }
};

struct EvalStackData
{
  ElemStars elemStars;
  Pos pos;
  vector<unsigned,Alloc<unsigned>> range, stride;
  unsigned numGraphs=1;

  EvalStackData(const ElemStars&  elemStars, const Pos& pos, unsigned numStars):
    elemStars(elemStars), pos(pos)
  {
    for (unsigned i=2; i<numStars; ++i)
      {
        range.push_back(min(unsigned(elemStars.size()),(i+1)));
        stride.push_back(numGraphs);
        numGraphs*=range.back();
      }
  }
};

struct EvalStack
{
  const EvalStackData& data;
  vector<linkRep,Alloc<linkRep>> stack;
  size_t stackTop=0;
  EvalStack(const EvalStackData& data): data(data), stack(data.pos.size())  { }

  // evaluate recipe encoded by the op bitset and index \a idx within enumeration of numGraphs
  linkRep evalRecipe(unsigned op, unsigned idx)
  {
    stackTop=0;
    for (unsigned p=0, opIdx=0, starIdx=2; p<2*data.pos.size()-1; ++p) // recipe.size()==2*data.pos.size()-1
      if (p<2)
        stack[stackTop++]=data.elemStars[p];
      else if (starIdx<data.pos.size() && data.pos[starIdx]==p) // push a star, according to idx
        {
          stack[stackTop++]=data.elemStars[(idx/data.stride[starIdx-2])%data.range[starIdx-2]];
          ++starIdx;
        }
      else
        {
          if (stackTop>1)
            {
              if (op&(1<<opIdx)) // set intersection
                {
                  auto v=stack[--stackTop];
                  stack[stackTop-1]&=v;
                }
              else                   // set union
                {
                  auto v=stack[--stackTop];
                  stack[stackTop-1]|=v;
                }
            }
          ++opIdx;
        }

//    if (stackTop==0)
//      {
//        for (auto op: recipe)
//          syclPrintf("%d ",op);
//        syclPrintf("\n");
//      }
    assert(stackTop>=1);
    return stack[0];
  }
};

// constant representing no graph at all
constexpr linkRep noGraph=~linkRep(0);

class OutputBuffer
{
  linkRep data[1000000];
  bool m_blown=false;
public:
  using size_type=unsigned;
  using iterator=linkRep*;
  // push_back is GPU/CPU thread-safe
  void push_back(linkRep x) {
    
    size_type curr=writePtr, next;
    if (curr>=sizeof(data))
      { // buffer full!
        m_blown=true;
        return;
      }
   
#ifdef SYCL_LANGUAGE_VERSION
    sycl::atomic_ref<size_type,sycl::memory_order::seq_cst,
                     sycl::memory_scope::device> writeIdx(writePtr);
    do
      {
        curr=writeIdx;
        next=curr+1;
      } while (!writeIdx.compare_exchange_weak(curr,next));
#else
    writePtr=curr+1;
#endif
    data[curr]=x;
  }
  linkRep operator[](size_t i) const {return data[i];}
  size_type size() const {return writePtr;}
  iterator begin() {return data;}
  iterator end() {return data+size();}
  bool blown() const {return m_blown;}
private:
  size_type writePtr=0;
};

#ifdef SYCL_LANGUAGE_VERSION
using Event=sycl::event;
#else
struct Event {
  static void wait(const vector<Event>&) {}
};
#endif

struct BlockEvaluator: public EvalStackData
{

  vector<EvalStack,Alloc<EvalStack>> block;
  // an output queue for the results
  OutputBuffer result;
  vector<linkRep,Alloc<linkRep>> alreadySeen; // sorted list of graphs already visited
  BlockEvaluator(unsigned blockSize, unsigned numStars, const Pos& pos, const ElemStars& elemStars):
    EvalStackData(elemStars, pos, numStars), block(blockSize,{*this})
  {}

  void eval(unsigned op, unsigned start, unsigned i)
  {
    if (i+start<numGraphs)
      {
        auto r=block[i].evalRecipe(op,i+start);
        if (!binary_search(alreadySeen.begin(),alreadySeen.end(),r))
          result.push_back(r);
      }
  }
  Event evalBlock(unsigned op, unsigned start) {
    // this loop to be parallelised
#ifdef SYCL_LANGUAGE_VERSION
    return ecolab::syclQ().parallel_for(size(),[=,this](auto i) {eval(op,start,i);});
//    for (unsigned i=0; i<size(); ++i)
//      cout<<result[i]<<" ";
//    cout<<endl;
#else
    for (size_t i=0; i<size(); ++i) eval(op,start,i);
    return {};
#endif
  }
  size_t size() const {return block.size();}
};

void StarComplexityGen::fillStarMap(unsigned maxStars)
{
  if (elemStars.empty()) return;
  // insert the single star graph
  starMap.emplace(elemStars[0],1);

  for (unsigned numStars=2; numStars<maxStars; ++numStars)
    {
      Pos pos(numStars);
      do
        {
          ecolab::DeviceType<BlockEvaluator> block(blockSize, numStars,pos,elemStars);
          vector<Event> events;
          //#ifdef SYCL_LANGUAGE_VERSION
          block->alreadySeen.clear();
          for (auto& k: starMap) block->alreadySeen.push_back(k.first);
          //#endif
          for (unsigned op=0; op<(1<<(numStars-1)); ++op)
            {
              for (unsigned i=0; i<block->numGraphs; i+=block->size())
                events.emplace_back(block->evalBlock(op, i));
            }
          Event::wait(events);
          if (block->result.blown())
            throw runtime_error("Output buffer blown");
              
          for (auto j: block->result)
            starMap.emplace(j, numStars);
//                  for (auto i: recipe)
//                    cout<<i<<",";
//                  cout<<endl;
        } while (pos.next());
    }
}

NautyRep toNautyRep(linkRep g, unsigned nodes)
{
  NautyRep n(nodes);
  for (unsigned i=0; i<nodes; ++i)
    for (unsigned j=0; j<i; ++j)
      if (g&(1<<(i*(i-1)/2+j)))
        n(i,j)=n(j,i)=true;
  return n;
}

linkRep toLinkRep(const NautyRep& g)
{
  linkRep l=0;
  // symmetrise as we go
  for (unsigned i=0; i<g.nodes(); ++i)
    for (unsigned j=0; j<g.nodes(); ++j)
      if (g(i,j))
        {
          if (j<i)
            l|=1<<(i*(i-1)/2+j);
          else if (i<j)
            l|=1<<(j*(j-1)/2+i);
        }
        // else ignore self-links
  return l;
}

void StarComplexityGen::canonicaliseStarMap()
{
  vector<linkRep> toErase;
  for (auto i=starMap.begin(); i!=starMap.end(); ++i)
    {
      auto c=toLinkRep(toNautyRep(i->first, elemStars.size()).canonicalise());
      if (c!=i->first)
        {
          auto& starC=starMap[c];
          starC=starC>0? min(starC, i->second): i->second;
          //i=starMap.erase(i);
          toErase.push_back(i->first);
        }
    }
  for (auto i: toErase) starMap.erase(i);
}
