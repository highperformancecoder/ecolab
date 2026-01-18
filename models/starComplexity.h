// hard code maximum number of nodes
constexpr unsigned maxNodes=22, maxStars=2*maxNodes-1;

#include "netcomplexity.h"

#include "vecBitSet.h"


template <class I>
class linkRepImpl
{
public:
  using Impl=I;
private:
  constexpr static unsigned size=maxNodes*(maxNodes-1)/(16*sizeof(Impl))+1;
  Impl data[linkRepImpl<I>::size];
  CLASSDESC_ACCESS(linkRepImpl);
public:
  unsigned op=0, idx=0; // cached recipe parameters
  linkRepImpl()=default;
  linkRepImpl(unsigned x) {
    static_assert(sizeof(data)>=sizeof(x));
    memcpy(data,&x,sizeof(x));
    memset(((char*)data)+sizeof(x),0,sizeof(data)-sizeof(x));
  }
  const linkRepImpl& operator|=(const linkRepImpl& x) {
    for (unsigned i=0; i<size; ++i) data[i]|=x.data[i];
    return *this;
  }
  linkRepImpl operator|(linkRepImpl x) {return x|=*this;}
  const linkRepImpl& operator&=(const linkRepImpl& x) {
    for (unsigned i=0; i<size; ++i) data[i]&=x.data[i];
    return *this;
  }
  linkRepImpl operator&(linkRepImpl x) {return x&=*this;}
  
  linkRepImpl operator~() const {
    linkRepImpl r;
    for (unsigned i=0; i<size; ++i) r.data[i]=~data[i];
    return r;
  }
  linkRepImpl operator<<(int n) {
    linkRepImpl r(0);
    auto d=div(n, int(8*sizeof(Impl)));
    for (unsigned i=0; i<size-d.quot; ++i)
      r.data[i+d.quot]=data[i]<<d.rem;
    return r;
  }
  /// mask out bits not used, give a node count
  linkRepImpl& maskOut(unsigned nodes) {
    unsigned numBits=nodes*(nodes-1)/2;
    // divide data up into chunks of size of unsigned
    auto d=div(numBits, int(8*sizeof(unsigned)));
    auto uData=reinterpret_cast<unsigned*>(data);
    uData[d.quot]&=(1<<d.rem)-1;
    // check that data is a multiple of unsigned
    static_assert(sizeof(data)%sizeof(unsigned)==0);
    for (unsigned i=d.quot+1; i<sizeof(data)/(sizeof(unsigned)); ++i)
      uData[i]=0;
    return *this;
  }
  // return true if there is a link between nodes i and j
  bool operator()(unsigned i,unsigned j) const {
    if (i==j) return false; // no self-links
    if (i<j) std::swap(i,j);
    auto d=div(i*(i-1)/2+j, int(8*sizeof(unsigned)));
    auto uData=reinterpret_cast<const unsigned*>(data);
    return uData[d.quot] & 1<<d.rem;
  }
  bool empty() const {
    for (unsigned i=0; i<size; ++i)
      if (data[i]) return false;
    return true;
  }
  bool operator==(const linkRepImpl& x) const {return operator<=>(x)==0;}
  auto operator<=>(const linkRepImpl& x) const {
    for (unsigned i=0; i<size; ++i)
      if (auto r=data[i]<=>x.data[i]; r!=0)
        return r;
    return std::strong_ordering::equal;
  }
  std::vector<unsigned> dataAsVector() const {
    auto start=reinterpret_cast<const unsigned*>(data);
    auto end=reinterpret_cast<const unsigned*>(data+size);
    return std::vector<unsigned>(start,end);
  }
  void dataFromVector(const std::vector<unsigned>& x) {
    memcpy(data,x.data(),std::min(size_t(size)*sizeof(Impl),x.size()*sizeof(x[0])));
  }
};

//#ifdef SYCL_LANGUAGE_VERSION
//using linkRep=linkRepImpl<VecBitSet<unsigned,4>>;
//#else
// on NUC, unsigned works best (32 bits)
using linkRep=linkRepImpl<unsigned>;
//#endif

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
      auto tmp=arg.dataAsVector();
      targ<<tmp;
    }
  };
    
  template <> struct access_json_unpack<linkRep> {
    template <class _CD_ARG_TYPE>
    void operator()(classdesc::json_pack_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)
    {
      std::vector<unsigned> tmp;
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
  std::map<linkRep,std::string> recipe; //stashed recipes
  ElemStars elemStars;
  void generateElementaryStars(unsigned nodes);
  // fills starMap with graphs of \a numStars
  void fillStarMap(unsigned numStars);
  void canonicaliseStarMap();
  /// return complement canonical graph
  linkRep complement(linkRep) const;
  unsigned symmStar(linkRep) const;
  GraphComplexity complexity(linkRep) const;

  /// return an upper bound on the number of stars in the link representation
  unsigned starUpperBound(const linkRep&) const;
  /// star upper bound using ABC library
  unsigned starUpperBoundABC(linkRep) const;
  
  /// random number generator
  ecolab::urand uni;
  
  /// randomly generate an ER graph, and return the starUpperBound and complexity
  GraphComplexity randomERGraph(unsigned nodes, unsigned links);
};

