/*
  @copyright Russell Standish 2000-2014
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef RANDOM_INIT_BASE_H
#define RANDOM_INIT_BASE_H
#include "classdesc.h"
#include <sstream>
#include <limits>
#include <cstdlib>
#include <cstdarg>

namespace classdesc
{

  class random_init_t
  {
  public:
    /// return a uniform random number in [0..1)
    virtual double rand() {return double(std::rand())/RAND_MAX;}
    virtual ~random_init_t() {}
  };

  template <class T> void random_init(random_init_t&, const string&, T&);
  // constant objects cannot be initialised 
  template <class T> void random_initp(random_init_t&, const string&, const T&)
  {}

  /// a post processing hook, allowing specialised postprocessing of
  /// the randomly initialised object to nromalised to a valid state
  template <class T> void random_init_normalise(T&);

  // basic types
  template <class T> typename
  enable_if<And<is_arithmetic<T>,Not<is_floating_point<T> > >, void>::T
  random_initp(random_init_t& r, const string&, T& a, dummy<0> dum=0)
  {
    a=T(std::numeric_limits<T>::max()*r.rand());
  } 

  // floating point types need to be truncated to account for rounding
  // in ASCII serialisations
  template <class T> typename
  enable_if<is_floating_point<T>, void>::T
  random_initp(random_init_t& r, const string&, T& a, dummy<0> dum=0)
  {
    std::ostringstream o;
    o<<T(std::numeric_limits<T>::max()*r.rand());
    std::istringstream is(o.str());
    is>>a;
  } 

  template <> inline 
  void random_init(random_init_t& r, const string&, bool& a)
  {
    a=r.rand()>=0.5;
  }

  // strings
  template <class T> 
  void random_init(random_init_t& r, const string& d, std::basic_string<T>& a)
  {
    // randomly fill up to 10 elements
    a.resize(10*r.rand());
    for (size_t i=0; i<a.size(); ++i)
      random_init(r,d,a[i]);
  }    

  // array handling
  template <class T>
  void random_init(random_init_t& r, const string& d, is_array ia, const T& a, 
            int ndims,size_t ncopies,...)
  {
    std::va_list ap;
    va_start(ap,ncopies);
    size_t cnt=ncopies;
    for (int i=ndims-2; i>=0; --i) cnt*=va_arg(ap,size_t);
    va_end(ap);
    for (size_t i=0; i<cnt; ++i)
      random_init(r,d,(&a)[i]);
  }
 
    
  /**
     handle enums
  */

  template <class T> void random_init(random_init_t& r, const string& d,
                                    Enum_handle<T> arg)
  {
    // randomly select from the range of valid enum values.
    int idx=int(enum_keys<T>().size()*r.rand());
    arg=enum_keysData<T>().keysData[idx].value;
  }


  /** standard container handling */
  template <class T> typename
  enable_if<is_sequence<T>, void>::T
  random_initp(random_init_t& r, const string& d, T& a, dummy<1> dum=0)
  {
    // randomly fill up to 10 elements
    a.resize(10*r.rand());
    for (typename T::iterator i=a.begin(); i!=a.end(); ++i)
      random_init(r,d,*i);
  }

  template <class T1, class T2> 
  void random_init(random_init_t& r, const string& d, std::pair<T1,T2>& a)
  {
    random_init(r,d+".first",a.first);
    random_init(r,d+".second",a.second);
  }

  template <class T> typename
  enable_if<is_associative_container<T>, void>::T
  random_initp(random_init_t& r, const string& d, T& a, dummy<1> dum=0)
  {
    int numElem=int(10*r.rand());
    for (int i=0; i<numElem; ++i)
      {
        typename T::value_type v;
        random_init(r,d,v);
        a.insert(v);
      }
  }

  // regular pointers
  template <class T> void random_initp(random_init_t&, const string&, T*) {}

  /*
    Method pointers
  */
  template <class C, class T>
  void random_init(random_init_t&, const string&, C&, T) {}

 /*
    const static support
  */
  template <class T>
  void random_init(random_init_t&, const string&, is_const_static, T) {}

  // static methods
  template <class T, class U>
  void random_init(random_init_t&, const string&, is_const_static, const T&, U) {}

  // Exclude
  template <class T>
  void random_init(random_init_t& targ, const string& desc, Exclude<T>& arg) {}

  template <class T>
  void random_init_onbase(random_init_t& targ, const string& desc, T& arg) 
  {random_init(targ,desc,arg);}
 

}

namespace classdesc_access
{
  template <class T> struct access_random_init;
}

using classdesc::random_init;
using classdesc::random_init_onbase;

#endif
