/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef XML_PACK_EPILOGUE_H
#define XML_PACK_EPILOGUE_H
#include <typeName_epilogue.h>
#include "polyAccessXMLPack.h"

namespace classdesc
{
  class xml_pack_t;
  class xml_unpack_t;
}

namespace classdesc_access
{
#ifdef XML_PACK_BASE_H
  template <class T> struct access_xml_pack
  {
    //This routine uses operator << if defined
    void operator()(classdesc::xml_pack_t& t,const classdesc::string& d, const T& a)
    {t.pack_notag(d,a);}
  };
#endif

#ifdef XML_UNPACK_BASE_H
  //This routine uses operator >> if defined
  template <class T> struct access_xml_unpack
  {
    void operator()(classdesc::xml_unpack_t& t,const classdesc::string& d,T& a)
    {t.unpack(d,a);}
  };
  // partial specialisation to discard unpacking to constant data types
  template <class T> struct access_xml_unpack<const T>
  {
    void operator()(classdesc::xml_unpack_t& t,const classdesc::string& d, const T& a)
    {
      T tmp;
      t.unpack(d,tmp);
    }
  };
#endif
}

namespace classdesc
{
  template <class T>
  struct AllOtherXMLPackpTypes: public 
  Not< 
    Or<
      Or<
        Or< is_sequence<T>, is_associative_container<T> >, 
        is_fundamental<T> 
        >,
      is_enum<T>
      >
    >
  {};
}

namespace classdesc
{
#ifdef XML_PACK_BASE_H
  template <class T> typename
  enable_if<AllOtherXMLPackpTypes<T>, void >::T
  xml_packp(xml_pack_t& t,const string& d, T& a, dummy<0> dum=0)
  {
    xml_pack_t::Tag tag(t,d);
    classdesc_access::access_xml_pack<T>()(t,d,a);
  }
#endif  

#ifdef XML_UNPACK_BASE_H
 template <class T> typename
  enable_if<AllOtherXMLPackpTypes<T>, void >::T
  xml_unpackp(xml_unpack_t& t,const string& d,T& a, dummy<0> dum=0)
  {
    classdesc_access::access_xml_unpack<T>()(t,d,a);
  }

  template <class T> typename
  enable_if<is_enum<T>, void >::T
  xml_unpackp(xml_unpack_t& t,const string& d,T& a, dummy<1> dum=0)
  {
    classdesc_access::access_xml_unpack<T>()(t,d,a);
  }

  // handle const arguments
  template <class T> 
  void xml_unpackp(xml_unpack_t& t,const string& d, const T& a)
  {}
#endif
}

#ifdef XML_PACK_BASE_H
template <class T> void xml_pack(classdesc::xml_pack_t& t,const classdesc::string& d, T& a)
{
  classdesc::xml_packp(t,d,a);
}
  
template <class T> void xml_pack(classdesc::xml_pack_t& t,const classdesc::string& d, const T& a)
{
  classdesc::xml_packp(t,d,const_cast<T&>(a));
}

template <class T> classdesc::xml_pack_t& operator<<(classdesc::xml_pack_t& t, const T& a)
{xml_pack(t,"root",const_cast<T&>(a)); return t;}

#endif
  
#ifdef XML_UNPACK_BASE_H
template <class T> void xml_unpack(classdesc::xml_unpack_t& t,const classdesc::string& d,T& a)
{
  classdesc::xml_unpackp(t,d,a);
}
  
template <class T> classdesc::xml_unpack_t& operator>>(classdesc::xml_unpack_t& t, T& a)
{xml_unpack(t,"root",a); return t;}
#endif

namespace classdesc_access
{
  namespace cd=classdesc;
  // support for polymorphic types, if loaded
#ifdef NEW_POLY_H
#ifdef XML_PACK_BASE_H
  template <class T> struct access_xml_pack<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
  template <class T, class B> struct access_xml_pack<cd::Poly<T,B> >
  {
    template <class U>
    void operator()(cd::xml_pack_t& t, const cd::string& d, U& a)
    {
      xml_pack(t,d,cd::base_cast<B>::cast(a));
    }
  };
#endif
#ifdef XML_UNPACK_BASE_H
  template <class T> struct access_xml_unpack<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
  template <class T, class B> struct access_xml_unpack<cd::Poly<T,B> >
  {
    template <class U>
    void operator()(cd::xml_unpack_t& t, const cd::string& d, U& a)
    {
      xml_unpack(t,d,cd::base_cast<B>::cast(a));
    }
  };
#endif
#endif

#ifdef POLYPACKBASE_H
#ifdef XML_PACK_BASE_H
  template <> struct access_xml_pack<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
  template <class T> struct access_xml_pack<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
#endif
#ifdef XML_UNPACK_BASE_H
   template <> struct access_xml_unpack<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
  template <class T> struct access_xml_unpack<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
#endif
#endif

#ifdef POLYJSONBASE_H
#ifdef XML_PACK_BASE_H
  template <> struct access_xml_pack<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
  template <class T> struct access_xml_pack<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
#endif
#ifdef XML_UNPACK_BASE_H
  template <> struct access_xml_unpack<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
  template <class T> struct access_xml_unpack<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
#endif
#endif

#ifdef POLYXMLBASE_H
#ifdef XML_PACK_BASE_H
  template <> struct access_xml_pack<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
  template <class T> struct access_xml_pack<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::xml_pack_t> {};
#endif
#ifdef XML_UNPACK_BASE_H
  template <> struct access_xml_unpack<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
  template <class T> struct access_xml_unpack<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::xml_unpack_t> {};
#endif
#endif

#ifdef REF_H
#ifdef XML_PACK_BASE_H
  template <class T> struct access_xml_pack<cd::ref<T> >
  {
    void operator()(cd::xml_pack_t& x, const cd::string& d, cd::ref<T>& a)
    {
      if (a) xml_pack(x,d,*a);
    }
  };
#endif

#ifdef XML_UNPACK_BASE_H
  template <class T> struct access_xml_unpack<cd::ref<T> >
  {
    void operator()(cd::xml_unpack_t& x, const cd::string& d, cd::ref<T>& a)
    {
      if (x.exists(d))
        xml_unpack(x,d,*a);
    }
  };
#endif
#endif

#ifdef XML_PACK_BASE_H
  template<class T>
  struct access_xml_pack<T*>
  {
    void operator()(cd::xml_pack_t& targ, const cd::string& desc, T*&)
    {
      throw cd::exception("xml_pack of pointers not supported");
    } 
  };
#endif

#ifdef XML_UNPACK_BASE_H
  template<class T>
  struct access_xml_unpack<T*>
  {
    void operator()(cd::xml_unpack_t& targ, const cd::string& desc, T*&)
    {
      throw cd::exception("xml_unpack of pointers not supported");
    } 
  };
#endif
}



#endif
