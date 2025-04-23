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

struct EvalStack
{
  Recipe recipe;
  vector<linkRep,Alloc<linkRep>> stack;
  size_t stackTop=0;
  const ElemStars& elemStars;
  /// initialises the stack with the tresults of evaluating the first \a pre elements of \a recipe
  EvalStack(const Recipe& recipe, const ElemStars& elemStars, size_t pre):
    recipe(recipe), stack(recipe.size()), elemStars(elemStars)
  {
    if (pre>recipe.size()) pre=recipe.size();
    if (pre) evalRecipe(0,pre);
  }

  linkRep evalRecipe(size_t start, size_t end)
  {
    for (auto op=recipe.begin()+start; op!=recipe.begin()+end; ++op)
      switch (*op)
        {
      default:
        assert(*op<elemStars.size());
        assert(stackTop<stack.size());
        stack[stackTop++]=elemStars[*op];
        break;
        case setUnion:
          if (stackTop>1)
            {
            auto v=stack[--stackTop];
            stack[stackTop-1]|=v;
            }
          break;
        case setIntersection:
        if (stackTop>1)
          {
            auto v=stack[--stackTop];
            stack[stackTop-1]&=v;
          }
        break;
        }
    if (stackTop==0)
      {
        for (auto op: recipe)
          syclPrintf("%d ",op);
        syclPrintf("\n");
      }
    assert(stackTop>=1);
    return stack[0];
  }
};

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
struct Event
{
  void wait() {}
};
#endif

struct BlockEvaluator
{
  vector<EvalStack,Alloc<EvalStack>> block;
  // implement an output queue for the results
  OutputBuffer result;
  unsigned numGraphs=1;
  vector<unsigned,Alloc<unsigned>> range, stride;
  vector<linkRep,Alloc<linkRep>> alreadySeen; // sorted list of graphs already visited
  Pos pos;
  BlockEvaluator(unsigned blockSize, unsigned numStars, const Pos& pos, const EvalStack& evalStack):
    block(blockSize,evalStack), pos(pos)
  {
    for (unsigned i=2; i<numStars; ++i)
      {
        range.push_back(min(unsigned(evalStack.elemStars.size()),(i+1)));
        stride.push_back(numGraphs);
        numGraphs*=range.back();
      }
  }
  void eval(size_t i, size_t idx) {
    for (unsigned j=2; j<pos.size(); ++j)
      block[i].recipe[pos[j]]=(idx/stride[j-2])%range[j-2];
    block[i].stackTop=0;
    auto r=block[i].evalRecipe(0, block[i].recipe.size());
    result.push_back(r);
//#ifdef SYCL_LANGUAGE_VERSION
//    if (binary_search(alreadySeen.begin(),alreadySeen.end(),result[i]))
//      result[i]=noGraph;
//#endif
  }
  Event evalBlock(size_t start) {
    // this loop to be parallelised
#ifdef SYCL_LANGUAGE_VERSION
    return ecolab::syclQ().parallel_for(size(),[=,this](auto i) {
      if (i+start<numGraphs)
        eval(i,i+start);
    });
//    for (unsigned i=0; i<size(); ++i)
//      cout<<result[i]<<" ";
//    cout<<endl;
#else
    for (size_t i=0; i<size(); ++i)
      if (i+start<numGraphs)
        eval(i, i+start);
    return {};
#endif
  }
  size_t size() const {return block.size();}
};

void StarComplexityGen::fillStarMap(unsigned maxStars)
{
  if (elemStars.empty()) return;
  // insert the single star graph
  starMap.emplace(EvalStack({0,setUnion},elemStars,2).stack.front(),1);

  // make a device accessible copy;
  ecolab::DeviceType<ElemStars> elemStars(this->elemStars);
  for (unsigned numStars=2; numStars<maxStars; ++numStars)
    {
      Pos pos(numStars);
      do
        {
          for (unsigned op=0; op<(1<<(numStars-1)); ++op)
            {
              Recipe recipe{0,1}; recipe.reserve(2*numStars-1);
              for (int i=2, opIdx=0, starIdx=2; i<2*numStars-1; ++i)
                if (pos[starIdx]==i)
                  {
                    recipe.push_back(0);
                    ++starIdx;
                  }
                else
                  {
                    if (op&(1<<opIdx))
                      recipe.push_back(setIntersection);
                    else
                      recipe.push_back(setUnion);
                    ++opIdx;
                  }

              EvalStack evalStack(recipe,*elemStars,0);
              ecolab::DeviceType<BlockEvaluator> block(blockSize, numStars,pos,evalStack);

              Event event;
              for (unsigned i=0; i<block->numGraphs; i+=block->size(), event.wait())
//#ifdef SYCL_LANGUAGE_VERSION
//                  block.alreadySeen.clear();
//                  for (auto& k: starMap) block.alreadySeen.push_back(k.first);
//#endif
                  event=block->evalBlock(i);
              event.wait();
              if (block->result.blown())
                throw runtime_error("Output buffer blown");
              
              for (auto j: block->result)
                starMap.emplace(j, numStars);
//                  for (auto i: recipe)
//                    cout<<i<<",";
//                  cout<<endl;
            }
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
