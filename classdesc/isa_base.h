/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
   \brief inheritance relationship descriptor

   This descriptor is deprecated in favour of std::tr1::::is_base_of
*/
#ifndef ISA_H
#define ISA_H
#include <typeinfo>
#include <set>
#include "classdesc.h"

namespace classdesc 
{
  /// data structure for storing inheritance relationships
  typedef  std::set<const std::type_info*> isa_t;
}

namespace classdesc_access
{
  template <class T> struct access_isa
  {
    void operator()(classdesc::isa_t& t,const classdesc::string& d, T& x) {}
  };
}
  
namespace classdesc 
{
  template <class T>
  void isa(classdesc::isa_t& t,const classdesc::string& d,T& x)
  {
    /* descend inheritance heirarchy only */
    if (d.length()==0) 
      {
        t.insert(&typeid(x));
        classdesc_access::access_isa<T>()(t,d,x);
      }
  }

  template <class T>
  void isa(classdesc::isa_t& t,const classdesc::string& d, const T& x)
  {isa(t,d,const_cast<T&>(x));}

  template <class T, class U>
  void isa(classdesc::isa_t& t,const classdesc::string& d, const T& x, const U& y) {}

  template <class T, class U, class V>
  //void isa(isa_t& t,classdesc::string d,const T& x, const U& y, const V& z) {}
  void isa(classdesc::isa_t& t,classdesc::string d,const T& x, const U& y, const V& z,...) {}

  template <class T>
  void isa_onbase(isa_t& t, const string& d,T& a)
  {isa(t,d,a);}
}
using classdesc::isa;
using classdesc::isa_onbase;

/**
  return whether \a trialT is derived from \a baseT
*/
template <class trialT, class baseT>
bool isa(const trialT& x,const baseT& y)
{
  classdesc::isa_t db;
  isa(db,classdesc::string(),const_cast<trialT&>(x));
  return db.count(&typeid(y));
}

#endif
