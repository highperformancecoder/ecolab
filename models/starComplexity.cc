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

struct BlockEvaluator
{
  vector<EvalStack,Alloc<EvalStack>> block;
  vector<linkRep,Alloc<linkRep>> result;
  unsigned numGraphs=1;
  vector<unsigned,Alloc<unsigned>> range, stride;
  vector<linkRep,Alloc<linkRep>> alreadySeen; // sorted list of graphs already visited
  Pos pos;
  BlockEvaluator(unsigned blockSize, unsigned numStars, const Pos& pos, const EvalStack& evalStack):
    block(blockSize,evalStack), result(blockSize,noGraph), pos(pos)
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
    result[i]=block[i].evalRecipe(0, block[i].recipe.size());
//#ifdef SYCL_LANGUAGE_VERSION
//    if (binary_search(alreadySeen.begin(),alreadySeen.end(),result[i]))
//      result[i]=noGraph;
//#endif
  }
  void evalBlock(size_t start) {
    // this loop to be parallelised
#ifdef SYCL_LANGUAGE_VERSION
    ecolab::syclQ().parallel_for(size(),[=,this](auto i) {
      eval(i,i+start);
    }).wait();
//    for (unsigned i=0; i<size(); ++i)
//      cout<<result[i]<<" ";
//    cout<<endl;
#else
    for (size_t i=0; i<size(); ++i)
      eval(i, i+start);
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

              for (unsigned i=0; i<block->numGraphs; i+=block->size())
                {
//#ifdef SYCL_LANGUAGE_VERSION
//                  block.alreadySeen.clear();
//                  for (auto& k: starMap) block.alreadySeen.push_back(k.first);
//#endif
                  block->evalBlock(i);
                  for (unsigned j=0; j<block->result.size(); ++j)
                    if (i+j<block->numGraphs && block->result[j]!=noGraph)
                      starMap.emplace(block->result[j], numStars);
//                  for (auto i: recipe)
//                    cout<<i<<",";
//                  cout<<endl;
                  }
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
