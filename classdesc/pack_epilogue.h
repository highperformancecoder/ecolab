/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef PACK_EPILOGUE_H
#define PACK_EPILOGUE_H

namespace classdesc_access
{
  namespace cd=classdesc;

  // const arg versions
  template <class T>
  struct access_pack<const T>
  {
    void operator()(cd::pack_t& buf, const cd::string& desc, const T& arg)
    {access_pack<T>()(buf,desc,arg);}
  };

  
  template <class T>
  struct access_unpack<const T>
  {
    void operator()(cd::pack_t& buf, const cd::string& desc, const T& arg)
    {access_unpack<T>()(buf,desc,arg);}
  };

  template <class T> typename 
  cd::enable_if<cd::is_sequence<T>,void>::T
  pack_container(cd::pack_t& buf,const cd::string& desc, T& arg,
                 cd::dummy<0> dum=0)
  {access_pack<cd::sequence<T> >()(buf,desc,arg);}

  template <class T> typename 
  cd::enable_if<cd::is_associative_container<T>,void>::T
  pack_container(cd::pack_t& buf,const cd::string& desc, T& arg,
                 cd::dummy<1> dum=0)
  {access_pack<cd::associative_container<T> >()(buf,desc,arg);}
  

  // handle containers from the generic access_ classes
  template <class T>
  struct access_pack
  {
    template <class U>
    void operator()(cd::pack_t& buf,const cd::string& desc, U& arg)
    {pack_container(buf,desc,arg);}
  };

  template <class T> typename 
  cd::enable_if<classdesc::is_sequence<T>,void>::T
  unpack_container(cd::pack_t& buf,const cd::string& desc, T& arg,
                   cd::dummy<0> dum=0)
  {access_unpack<classdesc::sequence<T> >()(buf,desc,arg);}

  template <class T> typename 
  cd::enable_if<classdesc::is_associative_container<T>,void>::T
  unpack_container(cd::pack_t& buf,const cd::string& desc, T& arg,
                   cd::dummy<1> dum=0)
  {access_unpack<classdesc::associative_container<T> >()(buf,desc,arg);}
  
  template <class T>
  struct access_unpack
  {
    template <class U>
    void operator()(cd::pack_t& buf,const cd::string& desc, U& arg)
    {unpack_container(buf,desc,arg);}
  };

  // support for polymorphic types, if loaded
#ifdef NEW_POLY_H
  template <class T> struct access_pack<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T> struct access_unpack<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T, class B> struct access_pack<cd::Poly<T,B> >
  {
    template <class U>
    void operator()(cd::pack_t& t, const cd::string& d, U& a)
    {
      pack(t,d,cd::base_cast<B>::cast(a));
    }
  };
  template <class T, class B> struct access_unpack<cd::Poly<T,B> > 
  {
    template <class U>
    void operator()(cd::pack_t& t, const cd::string& d, U& a)
    {
      unpack(t,d,cd::base_cast<B>::cast(a));
    }
  };
#endif

#ifdef POLYPACKBASE_H
  template <> struct access_pack<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <> struct access_unpack<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::unpack_t> {};
  template <class T> struct access_pack<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T> struct access_unpack<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::unpack_t> {};
#endif

#ifdef POLYJSONBASE_H
  template <> struct access_pack<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <> struct access_unpack<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::unpack_t> {};
  template <class T> struct access_pack<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T> struct access_unpack<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::unpack_t> {};
#endif

#ifdef POLYXMLBASE_H
  template <> struct access_pack<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <> struct access_unpack<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::unpack_t> {};
  template <class T> struct access_pack<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T> struct access_unpack<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::unpack_t> {};
#endif

#ifdef FACTORY_H
  template <class T, class U> struct access_pack<cd::Factory<T,U> > 
  {
    void operator()(cd::pack_t& b,const cd::string& d, cd::Factory<T,U>& a)
    {
      pack(b,d,a.fmap);
    }
  };

  

  template <class T, class U> struct access_unpack<cd::Factory<T,U> > 
  {
    void operator()(cd::unpack_t& b,const cd::string& d, cd::Factory<T,U>& a)
    {
      unpack(b,d,a.fmap);
    }
  };
#endif

}
#include "polyAccessPack.h"
#include "pack_stream.h"
#include "pack_stl.h"
#endif
