#include "starComplexity.h"
#include "starComplexity.cd"
#include "pythonBuffer.h"
#include "ecolab_epilogue.h"

#include <algorithm>
#include <assert.h>

StarComplexityGen starC;
CLASSDESC_ADD_GLOBAL(starC);
CLASSDESC_PYTHON_MODULE(starComplexity);

using namespace std;

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

bool next(Recipe& recipe)
{
  if (recipe.size()<3) return false;
  int nodeCtr=2; 
  for (auto i=recipe.begin()+2; i!=recipe.end(); ++i)
    {
      ++*i;
      if (*i>=nodeCtr) ++nodeCtr;
      if (*i>=nodeCtr) *i=setUnion; // wrap around
      // if i=0, we must bump the next op in the recipe
      if (*i!=0) return true;
    }
  
  return false;
}

void adjustToNumStars(Recipe& recipe, unsigned numStars)
{
  unsigned starsPresent=0;
  for (auto i=recipe.begin(); i!=recipe.end(); ++i)
    {
      starsPresent += *i>=0;
      if (starsPresent==numStars)
        recipe.erase(i, recipe.end());
    }
  int extraStars=0;
  for (; starsPresent<numStars; ++starsPresent)
    recipe.push_back(++extraStars);
  // ensure recipe ends with an op
  if (recipe.back()>=0)
    recipe.push_back(setUnion);
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

void StarComplexityGen::fillStarMap(unsigned numStars)
{
  Recipe recipe(numStars,0);
  if (numStars>1) recipe[1]=1;
  recipe.emplace_back(setUnion);
  
  do
    // assume we're filling starMap in numStar order
    starMap.emplace(evalRecipe(recipe,elemStars),numStars);
  while (next(recipe));
}
