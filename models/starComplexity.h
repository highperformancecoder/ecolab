using linkRep=unsigned long long;

#include <map>
#include <unordered_map>
#include <vector>

#include "ecolab.h"

using ElemStars=std::vector<linkRep>;
struct GraphComplexity
{
  double starComplexity;
  double complexity;
};

struct StarComplexityGen
{
  unsigned maxNumGraphs=100000000;
  size_t blockSize=128;
  // star complexity registry
  std::map<linkRep,unsigned> starMap;
  std::map<linkRep,unsigned> counts; // counts the number of times linkRep is seen
  ElemStars elemStars;
  void generateElementaryStars(unsigned nodes);
  // fills starMap with graphs of \a numStars
  void fillStarMap(unsigned maxStars);
  void canonicaliseStarMap();
  /// return complement canonical graph
  linkRep complement(linkRep) const;
  unsigned symmStar(linkRep) const;
  GraphComplexity complexity(linkRep) const;
};

