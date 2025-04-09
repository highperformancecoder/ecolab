#include "starComplexity.h"
#include "starComplexity.cd"
#include "pythonBuffer.h"
#include "netcomplexity.h"
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
  return linkRep(1)<<i*(i-1)/2+j;
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
using Recipe=vector<int>;

void adjustToNumStars(Recipe& recipe, unsigned numStars)
{
  unsigned starsPresent=0;
  int finalOp=setUnion;
  for (auto i=recipe.begin(); i!=recipe.end(); ++i)
    {
      starsPresent += *i>=0;
      if (starsPresent>numStars)
        {
          recipe.erase(i, recipe.end());
          break;
        }
    }
  int extraStars=0;
  for (; starsPresent<numStars; ++starsPresent)
    recipe.push_back(0);
  // ensure recipe ends with an op
  if (recipe.back()>=0)
    recipe.push_back(setUnion);
}

inline unsigned countStars(const Recipe& recipe)
{
  return accumulate(recipe.begin(), recipe.end(), 0U,
                    [](unsigned a, int i) {return a+(i>=0);});
}

bool next(Recipe& recipe, int nodes, int maxNumStars)
{
  //const unsigned maxNumStars=7; //nodes*(nodes-1)-1;
  if (recipe.size()<3)
    return false;
  int nodeCtr=2;
  int numStars=2, numOps=0;
  auto i=recipe.begin()+2;
  for (; i!=recipe.end(); ++i)
    {
      ++*i;
      if (*i<nodes && *i>=nodeCtr) ++nodeCtr;
      bool wrappedAround=*i>=nodeCtr;
      if (wrappedAround)
        if (numStars>numOps+1)
          *i=setUnion;
        else
          *i=0; // no point putting a set operation here, it would be nop
      
      if (*i<0)
        ++numOps;
      else
        ++numStars;

      if (numStars>maxNumStars)
        return false;

      if (!wrappedAround)
        {
          // count remaining ops and stars, and balance with ops
          for (; i!=recipe.end(); ++i)
            if (*i<0)
              ++numOps;
            else
              ++numStars;
          for (int i=0; i<numStars-numOps-1; ++i)
            recipe.push_back(setUnion);
          return true;
        }
    }
  // if we've reached the end, then we need to add operations to balance the stack
  if (numStars<=maxNumStars)
    {
      for (int i=0; i<numStars-numOps-1; ++i)
        recipe.push_back(setUnion);
      return true;
    }
  return false;
}

linkRep evalRecipe(const Recipe& recipe, const std::vector<linkRep>& elemStars)
{
  vector<linkRep> stack;
  for (auto op: recipe)
    switch (op)
      {
      default:
        assert(op<elemStars.size());
        stack.push_back(elemStars[op]);
        break;
      case setUnion:
        if (stack.size()>1)
          {
            auto v=stack.back();
            stack.pop_back();
            stack.back()|=v;
          }
        break;
      case setIntersection:
        if (stack.size()>1)
          {
            auto v=stack.back();
            stack.pop_back();
            stack.back()&=v;
          }
        break;
      }
  assert(recipe.back()<0);
  // apply last operation to rest of the stack
  if (stack.size()>1)
    switch (recipe.back())
      {
      case setUnion:
        for (auto& i: stack)
          stack.back()|=i;
        break;
      case setIntersection:
        for (auto& i: stack)
          stack.back()&=i;
        break;
      default:
        assert(false);
      }
  return stack.back();
}

void StarComplexityGen::fillStarMap(unsigned maxStars)
{
  if (elemStars.empty()) return;
  // insert the single star graph
  starMap.emplace(evalRecipe({0,setUnion},elemStars),1);

  Recipe recipe{0,1,setUnion};
  do
    {
      // assume we're filling starMap in numStar order
//      for (auto i: recipe)
//        cout<<i<<",";
//      cout<<endl;
      auto& starC=starMap[evalRecipe(recipe,elemStars)];
      starC = starC? min(starC, countStars(recipe)): countStars(recipe);
    }
  while (next(recipe,elemStars.size(),maxStars));
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
        if (j<i)
          l|=1<<(i*(i-1)/2+j);
        else if (i<j)
          l|=1<<(j*(j-1)/2+i);
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
