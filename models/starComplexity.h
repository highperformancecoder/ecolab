using linkRep=unsigned;

#include <map>
#include <unordered_map>
#include <vector>

#include "ecolab.h"

#ifdef SYCL_LANGUAGE_VERSION
template <class T>
using Alloc=ecolab::SyclQAllocator<T, sycl::usm::alloc::shared>;
#else
template <class T> using Alloc=std::allocator<T>;
#endif

using ElemStars=std::vector<linkRep>;

struct StarComplexityGen
{
  unsigned maxNumGraphs=100000000;
  size_t blockSize=128;
  // star complexity registry
  std::map<linkRep,unsigned> starMap;
  ElemStars elemStars;
  void generateElementaryStars(unsigned nodes);
  // fills starMap with graphs of \a numStars
  void fillStarMap(unsigned maxStars);
  void canonicaliseStarMap();
};

