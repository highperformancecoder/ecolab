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
struct Recipe: public vector<int>
{
  // maintained within next, and when first initialised.
  int numStars=0, numOps=0;
  Recipe()=default;
  Recipe(const initializer_list<int>& args): vector<int>(args)
  {
    for (auto& i: *this)
      if (i<0)
        ++numOps;
      else
        ++numStars;
  }
};

inline unsigned countStars(const Recipe& recipe)
{
  return accumulate(recipe.begin(), recipe.end(), 0U,
                    [](unsigned a, int i) {return a+(i>=0);});
}

// @return number of stars in result, 0 if finished
int next(Recipe& recipe, int nodes, int maxNumStars)
{
  //const unsigned maxNumStars=7; //nodes*(nodes-1)-1;
  if (recipe.size()<3)
    return false;

 tryAgain:
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
          {
            *i=setUnion;
            --recipe.numStars;
            ++recipe.numOps;
          }
        else
          *i=0; // no point putting a set operation here, it would be nop
      
      if (*i<0)
        ++numOps;
      else
        ++numStars;

      if (!wrappedAround)
        {
          if (*i==0) // gone from op to star
            {
              ++recipe.numStars;
              --recipe.numOps;
            }
          if (recipe.numStars>maxNumStars) goto tryAgain;
          for (; recipe.numStars>recipe.numOps+1 || recipe.back()>=0; ++recipe.numOps)
            recipe.push_back(setUnion);
          return numStars;
        }
    }
  return 0;
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
      starC = starC? min(starC, unsigned(recipe.numStars)): recipe.numStars;
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
