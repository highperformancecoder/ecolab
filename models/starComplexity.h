using linkRep=unsigned;

#include <map>
#include <vector>

struct StarComplexityGen
{
  unsigned maxNumGraphs=100000000;
  // star complexity registry
  std::map<linkRep,unsigned> starMap;
  std::vector<linkRep> elemStars;
  void generateElementaryStars(unsigned nodes);
  // fills starMap with graphs of \a numStars
  void fillStarMap(unsigned numStars);
  void canonicaliseStarMap() {}
};

