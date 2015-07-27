/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLY_ACCESS_PACK_H
#define POLY_ACCESS_PACK_H
#include "polyPackBase.h"

namespace classdesc
{
#ifdef POLYPACKBASE_H
  template <class T>
  typename enable_if<is_base_of<PolyPackBase, typename T::element_type> >::T
  pack_smart_ptr(pack_t& x, const string& d, const T& a, 
                  dummy<0> dum=0)
  {
    bool valid=a.get();
    ::pack(x,d,valid);
    if (valid)
      {
        typename T::element_type::Type t=a->type();
        ::pack(x,d,t);
        a->pack(x,d);
      }
  }

  template <class T>
  typename enable_if<Not<is_base_of<PolyPackBase, typename T::element_type> > >::T
#else
  template <class T>
  void //if Poly not defined, just define shared_ptr non-polymorphically
#endif
  pack_smart_ptr(pack_t& x, const string& d, const T& a, 
                    dummy<1> dum=0)
  {
    bool valid=a.get();
    pack(x,d,valid);
    if (valid)
      pack(x,d,*a);
  }
  
#ifdef POLYPACKBASE_H
  template <class T>
  typename enable_if<is_base_of<PolyPackBase, typename T::element_type> >::T
  unpack_smart_ptr(unpack_t& x, const string& d, T& a, dummy<0> dum=0)
  {
    bool valid;
    unpack(x,d,valid);
    if (valid)
      {
        typename T::element_type::Type t;
        unpack(x,d,t);
        a.reset(T::element_type::create(t));
        a->unpack(x,d);
      }
    else
      a.reset();
  }
  
  template <class T>
  typename enable_if<Not<is_base_of<PolyPackBase, typename T::element_type> > >::T
#else
  template <class T>
  void //if Poly not defined, just define smart_ptr non-polymorphically
#endif
  unpack_smart_ptr(pack_t& x, const string& d, T& a, dummy<1> dum=0)
  {
    bool valid;
    unpack(x,d,valid);
    if (valid)
      {
        a.reset(new typename T::element_type);
        unpack(x,d,*a);
      }
    else
      a.reset();
  }
  
}

namespace classdesc_access
{
  namespace cd = classdesc;

  template <class T>
  struct access_pack<cd::shared_ptr<T> >
  {
    template <class U>
    void operator()(cd::pack_t& x, const cd::string& d, U& a)
    {pack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_unpack<cd::shared_ptr<T> >
  {
    template <class U>
    void operator()(cd::unpack_t& x, const cd::string& d, U& a)
    {unpack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_pack<std::auto_ptr<T> >
  {
    template <class U>
    void operator()(cd::pack_t& x, const cd::string& d, U& a)
    {pack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_unpack<std::auto_ptr<T> >
  {
    template <class U>
    void operator()(cd::unpack_t& x, const cd::string& d, U& a)
    {unpack_smart_ptr(x,d,a);}
  };

#if defined(__cplusplus) && __cplusplus >= 201103L
  template <class T, class D>
  struct access_pack<std::unique_ptr<T,D>>
  {
    template <class U>
    void operator()(cd::pack_t& x, const cd::string& d, U& a)
    {pack_smart_ptr(x,d,a);}
  };

  template <class T, class D>
  struct access_unpack<std::unique_ptr<T,D>>
  {
    template <class U>
    void operator()(cd::unpack_t& x, const cd::string& d, U& a)
    {unpack_smart_ptr(x,d,a);}
  };
#endif

}

#endif
