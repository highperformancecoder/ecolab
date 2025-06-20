// hard code maximum number of nodes
constexpr unsigned maxNodes=22, maxStars=2*maxNodes-1;

//using linkRep=unsigned long long;

#include "netcomplexity.h"

#include "vecBitSet.h"



class linkRep
{
public:
#ifdef SYCL_LANGUAGE_VERSION
  using Impl=VecBitSet<unsigned,4>;
#else
  using Impl=/*long long*/ unsigned;
#endif
private:
  constexpr static unsigned size=maxNodes*(maxNodes-1)/(16*sizeof(Impl))+1;
  Impl data[linkRep::size];
  CLASSDESC_ACCESS(linkRep);
public:
  linkRep()=default;
  linkRep(unsigned long long x) {
    static_assert(sizeof(data)>=sizeof(x));
    memcpy(data,&x,sizeof(x));
    memset(((char*)data)+sizeof(x),0,sizeof(data)-sizeof(x));
  }
  const linkRep& operator|=(const linkRep& x) {
    for (unsigned i=0; i<size; ++i) data[i]|=x.data[i];
    return *this;
  }
  linkRep operator|(linkRep x) {return x|=*this;}
  const linkRep& operator&=(const linkRep& x) {
    for (unsigned i=0; i<size; ++i) data[i]&=x.data[i];
    return *this;
  }
  linkRep operator&(linkRep x) {return x&=*this;}
  
  linkRep operator~() const {
    linkRep r;
    for (unsigned i=0; i<size; ++i) r.data[i]=~data[i];
    return r;
  }
  linkRep operator<<(int n) {
    linkRep r;
    auto d=div(n, int(8*sizeof(Impl)));
    for (unsigned i=0; i<size-d.quot; ++i)
      r.data[i+d.quot]=data[i]<<d.rem;
    return r;
  }
  operator const void*() const {
    for (unsigned i=0; i<size; ++i)
      if (data[i]) return data;
    return nullptr;
  }

  bool operator<(const linkRep& x) const {
    for (unsigned i=0; i<size; ++i)
      {
        if (data[i]<x.data[i]) return true;
        if (x.data[i]<data[i]) return false;
      }
    return false;
  }
  std::vector<linkRep::Impl> dataAsVector() const {
    return std::vector<Impl>(data,data+size);
  }
  void dataFromVector(const std::vector<linkRep::Impl>& x) {
    memcpy(data,x.data(),std::min(size_t(size),x.size())*sizeof(linkRep::Impl));
  }
};

// Convert to/from a JSON array for Python conversion
#define CLASSDESC_json_pack___linkRep
#define CLASSDESC_json_unpack___linkRep

#include "json_pack_base.h"
namespace classdesc_access
{
  template <> struct access_json_pack<linkRep> {
    template <class _CD_ARG_TYPE>
    void operator()(classdesc::json_pack_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)
    {
      std::vector<linkRep::Impl> tmp=arg.dataAsVector();
      targ<<tmp;
    }
  };
    
  template <> struct access_json_unpack<linkRep> {
    template <class _CD_ARG_TYPE>
    void operator()(classdesc::json_pack_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)
    {
      std::vector<linkRep::Impl> tmp;
      targ>>tmp;
      arg.dataFromVector(tmp);
    }
  };
}

static_assert(sizeof(linkRep)*8 > (maxNodes*(maxNodes-1))/2);

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

