// a type of high bitcount objects suitable for SYCL

#ifndef ECOLAB_VECBITSET_H
#define ECOLAB_VECBITSET_H

#ifdef SYCL_LANGUAGE_VERSION
#include <sycl/sycl.hpp>

// constructs a swizzle permutation (From...V::size(), 0...From-1)
// initialise with To=V::size(), NumItems=0 and N=
// NumItems tracks the numer of items in the parameter pack \a N
template <class V, unsigned From, unsigned To, unsigned NumItems, unsigned... N>
V swizzleFrom(const V& x) {
  if constexpr (From<To) {
    return swizzleFrom<V,From+1,To, NumItems+1, N...,From>(x);
  } else if constexpr (NumItems<V::size()) { // fill in remainder of permutation
    return swizzleFrom<V,0,V::size()-NumItems,NumItems,N...>(x);
  } else {
    return V(x.template swizzle<N...>());
  }
}

template <class V, unsigned From>
V swizzleFrom(const V& x) {return swizzleFrom<V,From,unsigned(V::size()),0>(x);}

/// assign the value \a t to all elements [From..To)
template <class V, class T, unsigned From, unsigned To, unsigned... N>
void swizzleAsg(V& x, const T& t) {
  if constexpr (From<To) {
    swizzleAsg<V,T,From+1,To, N...,From>(x,t);
  } else {
    x.template swizzle<N...>()=t;
  }
}

// a type of high bitcount objects
template <class T, unsigned N>
class VecBitSet: public sycl::vec<T,N>
{
  // shift vector by n positions using a swizzle
  template <unsigned n> VecBitSet shiftLeft() const {
    if constexpr (n<N) {
      VecBitSet r=swizzleFrom<const VecBitSet,N-n>(*this);
      swizzleAsg<VecBitSet,T,0,n>(r,0);
      return r;
    }
    return VecBitSet(0);
  }
  
  // shift vector by n positions using a swizzle
  template <unsigned n> VecBitSet shiftRight() const {
    if constexpr (n<N) {
      VecBitSet r=swizzleFrom<const VecBitSet,n>(*this);
      swizzleAsg<VecBitSet,T,N-n,N>(r,0);
      return r;
    }
    return VecBitSet(0);
  }

  // returns result of bitshifting to the left
  // @param r = original vector shifted one extra posiution to the left
  // @param l = original vector 
  VecBitSet bitShiftLeft(sycl::vec<T,N> r, const sycl::vec<T,N>& l, unsigned remainder) const {
    r>>=(8*sizeof(T)-remainder);
    r|=l<<remainder;
    return VecBitSet(r);
  }

#ifdef CLASSDESC_ACCESS
  CLASSDESC_ACCESS(VecBitSet);
#endif
public:
  using Vec=sycl::vec<T,N>;
  VecBitSet()=default;
  template <class U>
  explicit VecBitSet(const U& x): sycl::vec<T,N>(x) {}
  explicit VecBitSet(Vec&& x): sycl::vec<T,N>(std::move(x)) {}
  VecBitSet& operator=(T x) {sycl::vec<T,N>::operator=(x); return *this;}
  VecBitSet& operator=(const Vec& x) {sycl::vec<T,N>::operator=(x); return *this;}
  operator bool() const {
    for (unsigned i=0; i<N; ++i)
      if ((*this)[i]) return true;
    return false;
  }
  
  // only need to fix up bit shift operations
  VecBitSet operator<<(int n) const {
    auto d=div(n,8*sizeof(T));
    switch (d.quot) {
    case 0: return bitShiftLeft(shiftLeft<1>(), *this, d.rem);
    case 1: return bitShiftLeft(shiftLeft<2>(), shiftLeft<1>(), d.rem);
    case 2: return bitShiftLeft(shiftLeft<3>(), shiftLeft<2>(), d.rem);
    case 3: return bitShiftLeft(shiftLeft<4>(), shiftLeft<3>(), d.rem);
    case 4: return bitShiftLeft(shiftLeft<5>(), shiftLeft<4>(), d.rem);
    case 5: return bitShiftLeft(shiftLeft<6>(), shiftLeft<5>(), d.rem);
    case 6: return bitShiftLeft(shiftLeft<7>(), shiftLeft<6>(), d.rem);
    case 7: return bitShiftLeft(shiftLeft<8>(), shiftLeft<7>(), d.rem);
    case 8: return bitShiftLeft(shiftLeft<9>(), shiftLeft<8>(), d.rem);
    case 9: return bitShiftLeft(shiftLeft<10>(), shiftLeft<9>(), d.rem);
    case 10: return bitShiftLeft(shiftLeft<11>(), shiftLeft<10>(), d.rem);
    case 11: return bitShiftLeft(shiftLeft<12>(), shiftLeft<11>(), d.rem);
    case 12: return bitShiftLeft(shiftLeft<13>(), shiftLeft<12>(), d.rem);
    case 13: return bitShiftLeft(shiftLeft<14>(), shiftLeft<13>(), d.rem);
    case 14: return bitShiftLeft(shiftLeft<15>(), shiftLeft<14>(), d.rem);
    case 15: return shiftLeft<15>() >> int(8*sizeof(T)-d.rem);
    default: return VecBitSet(0); // SYCL defines a maximum of 16 elements in a vec.
    }
  }
  VecBitSet operator>>(int n) const {
    auto d=div(n,8*sizeof(T));
    d.rem=(8*sizeof(T)-d.rem)%(8*sizeof(T)); // shift the other way
    switch (d.quot) {
    case 0: return bitShiftLeft(*this, shiftRight<1>(), d.rem);
    case 1: return bitShiftLeft(shiftRight<1>(), shiftRight<2>(), d.rem);
    case 2: return bitShiftLeft(shiftRight<2>(), shiftRight<3>(), d.rem);
    case 3: return bitShiftLeft(shiftRight<3>(), shiftRight<4>(), d.rem);
    case 4: return bitShiftLeft(shiftRight<4>(), shiftRight<5>(), d.rem);
    case 5: return bitShiftLeft(shiftRight<5>(), shiftRight<6>(), d.rem);
    case 6: return bitShiftLeft(shiftRight<6>(), shiftRight<7>(), d.rem);
    case 7: return bitShiftLeft(shiftRight<7>(), shiftRight<8>(), d.rem);
    case 8: return bitShiftLeft(shiftRight<8>(), shiftRight<9>(), d.rem);
    case 9: return bitShiftLeft(shiftRight<9>(), shiftRight<10>(), d.rem);
    case 10: return bitShiftLeft(shiftRight<10>(), shiftRight<11>(), d.rem);
    case 11: return bitShiftLeft(shiftRight<11>(), shiftRight<12>(), d.rem);
    case 12: return bitShiftLeft(shiftRight<12>(), shiftRight<13>(), d.rem);
    case 13: return bitShiftLeft(shiftRight<13>(), shiftRight<14>(), d.rem);
    case 14: return bitShiftLeft(shiftRight<14>(), shiftRight<15>(), d.rem);
    case 15: return shiftRight<15>() >> d.rem;
    default: return VecBitSet(0); // SYCL defines a maximum of 16 elements in a vec.
    }
  }

  bool operator==(const VecBitSet& x) const {
    for (unsigned i=0; i<N; ++i)
      if ((*this)[i]!=x[i])
        return false;
    return true;
  }
  auto operator<=>(const VecBitSet& x) const {
    for (unsigned i=0; i<N; ++i)
      if (auto r=(*this)[i]<=>x[i]; r!=0)
        return r;
    return std::strong_ordering::equal;
  }
};

#define CLASSDESC_json_pack___VecBitSet_T_N_
#define CLASSDESC_json_unpack___VecBitSet_T_N_
#define CLASSDESC_RESTProcess___VecBitSet_T_N_

#include "json_pack_base.h"
namespace classdesc_access
{
  template <class T, unsigned N> struct access_json_pack<VecBitSet<T,N>> {
    template <class _CD_ARG_TYPE>
    void operator()(classdesc::json_pack_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)
    {
      std::vector<T> tmp;
      for (unsigned i=0; i<arg.size(); ++i)
        tmp.push_back(arg[i]);
      targ<<tmp;
    }
  };
    
  template <class T, unsigned N> struct access_json_unpack<VecBitSet<T,N>> {
    template <class _CD_ARG_TYPE>
    void operator()(classdesc::json_pack_t& targ, const classdesc::string& desc,_CD_ARG_TYPE& arg)
    {
      std::vector<T> tmp;
      targ>>tmp;
      for (unsigned i=0; i<arg.size() && i<tmp.size(); ++i)
        arg[i]=tmp[i];
    }
  };

  template <class T, unsigned N> struct access_RESTProcess<VecBitSet<T,N>>:
    public classdesc::NullDescriptor<classdesc::RESTProcess_t> {};
}

#endif
#endif
