using linkRep=unsigned;

#include <map>
#include <unordered_map>
#include <vector>

#include "ecolab.h"

using ElemStars=std::vector<linkRep>;
struct GraphComplexity
{
  unsigned star;
  double complexity;
};

struct StarComplexityGen
{
  unsigned maxNumGraphs=100000000;
  size_t blockSize=128;
  // star complexity registry
  std::map<linkRep,GraphComplexity> starMap;
  ElemStars elemStars;
  void generateElementaryStars(unsigned nodes);
  // fills starMap with graphs of \a numStars
  void fillStarMap(unsigned maxStars);
  void canonicaliseStarMap();
  /// return lesser of star and star of complement
  unsigned symmStar(linkRep);
};

