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

inline unsigned countStars(const Recipe& recipe)
{
  return accumulate(recipe.begin(), recipe.end(), 0U,
                    [](unsigned a, int i) {return a+(i>=0);});
}

//// @return number of stars in result, 0 if finished
//int next(Recipe& recipe, int nodes, int maxNumStars)
//{
//  //const unsigned maxNumStars=7; //nodes*(nodes-1)-1;
//  if (recipe.size()<3)
//    return false;
//
// tryAgain:
//  //int nodeCtr=2;
//  int numStars=2, numOps=0;
//  auto i=recipe.begin()+2;
//  for (; i!=recipe.end(); ++i)
//    {
//      if (*i>=0)
//        {
//        }
//      ++*i;
//      bool wrappedAround=*i>0;
//      if (wrappedAround)
//        if (numStars>numOps+1)
//          {
//            *i=setUnion;
//            --recipe.numStars;
//            ++recipe.numOps;
//          }
//        else
//          *i=0; // no point putting a set operation here, it would be nop
//      //if (*i>=0) *i=nodeCtr++;
//      
//      if (*i<0)
//        ++numOps;
//      else
//        ++numStars;
//
//      if (!wrappedAround)
//        {
//          if (*i>=0) // gone from op to star
//            {
//              ++recipe.numStars;
//              --recipe.numOps;
//            }
//          if (recipe.numStars>maxNumStars) goto tryAgain;
//          for (; recipe.numStars>recipe.numOps+1 || recipe.back()>=0; ++recipe.numOps)
//            recipe.push_back(setUnion);
//          return numStars;
//        }
//    }
//  return 0;
//}

linkRep evalRecipe(const Recipe& recipe, const std::vector<linkRep>& elemStars)
{
  linkRep stack[recipe.size()];
  size_t stackTop=0;
  for (auto op: recipe)
    switch (op)
      {
      default:
        assert(op<elemStars.size());
        stack[stackTop++]=elemStars[op];
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
  assert(stackTop==1);
  return stack[0];
}

// structure holding position vector of stars within a recipe
struct Pos: public vector<int>
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

void StarComplexityGen::fillStarMap(unsigned maxStars)
{
  if (elemStars.empty()) return;
  // insert the single star graph
  starMap.emplace(evalRecipe({0,setUnion},elemStars),1);



  for (unsigned numStars=2; numStars<maxStars; ++numStars)
    {
      // compute number of star combinations in a formula
      unsigned numGraphs=1;
      for (unsigned i=2; i<numStars; ++i) numGraphs*=(i+1);

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
              // now fill in star details. Parallelise this loop.
              for (unsigned i=0; i<numGraphs; ++i)
                {
//                  for (auto i: recipe)
//                    cout<<i<<",";
//                  cout<<endl;
                  starMap.emplace(evalRecipe(recipe,elemStars), numStars);
                  unsigned s=2;
                  for (auto p=recipe.begin()+2; p!=recipe.end(); ++p)
                    if (*p >= 0) 
                      {
                        ++*p;
                        if (*p<=s) break;
                        *p=0; // overflow to next
                        if (s<elemStars.size()-1) ++s;
                      }
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
