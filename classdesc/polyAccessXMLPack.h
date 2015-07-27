/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLY_ACCESS_XML_PACK_H
#define POLY_ACCESS_XML_PACK_H

#ifdef XML_PACK_BASE_H
namespace classdesc
{
#ifdef POLYXMLBASE_H
  // polymorphic version
  template <class T>
  typename enable_if<is_base_of<PolyXMLBase, typename T::element_type>, void>::T
  xml_pack_smart_ptr(xml_pack_t& x, const string& d, T& a) 
  {
    if (a.get())
      {
        // requires a bit of jiggery-pokery to get the tag wrappers
        // happening in the right order
        xml_pack_t::Tag t(x,d);
        ::xml_pack(x,d+".type",a->type());
        a->xml_pack(x,d);
      }
  }

  // non polymorphic version
  template <class T>
  typename enable_if<Not<is_base_of<PolyXMLBase, typename T::element_type> >, void>::T
#else
  template <class T>
  void
#endif
  xml_pack_smart_ptr(xml_pack_t& x, const string& d, T& a)
  {
    if (a)
      ::xml_pack(x,d,*a);
  }
  
}

// special handling of shared pointers to avoid a double wrapping problem
template<class T>
void xml_pack(classdesc::xml_pack_t& x, const classdesc::string& d, 
              classdesc::shared_ptr<T>& a)
{classdesc::xml_pack_smart_ptr(x,d,a);}

template<class T>
void xml_pack(classdesc::xml_pack_t& x, const classdesc::string& d, 
              std::auto_ptr<T>& a)
{classdesc::xml_pack_smart_ptr(x,d,a);}

#if defined(__cplusplus) && __cplusplus >= 201103L
template<class T, class D>
void xml_pack(classdesc::xml_pack_t& x, const classdesc::string& d, 
              std::unique_ptr<T,D>& a)
{classdesc::xml_pack_smart_ptr(x,d,a);}
#endif

#endif

#ifdef XML_UNPACK_BASE_H
namespace classdesc
{
#ifdef POLYXMLBASE_H
    // polymorphic version
  template <class T>
  typename enable_if<is_base_of<PolyXMLBase, typename T::element_type>, void>::T
  xml_unpack_smart_ptr(xml_unpack_t& x, const string& d, T& a, 
                        dummy<0> dum=0)
  {
    if (x.exists(d+".type"))
      {
        typename T::element_type::Type type;
        ::xml_unpack(x,d+".type",type);
        a.reset(T::element_type::create(type));
        a->xml_unpack(x,d);
      }
    else
      a.reset();
  }

    // non polymorphic version
  template <class T>
  typename enable_if<Not<is_base_of<PolyXMLBase, typename T::element_type> >, void>::T
#else
  template <class T>
  void
#endif
  xml_unpack_smart_ptr(xml_unpack_t& x, const string& d, 
                        T& a, dummy<1> dum=0)
  {
    if (x.exists(d))
      {
        a.reset(new typename T::element_type);
        ::xml_unpack(x,d,*a);
      }
    else
      a.reset();
  }
}

namespace classdesc_access
{
  namespace cd = classdesc;

  template <class T>
  struct access_xml_unpack<cd::shared_ptr<T> >
  {
    void operator()(cd::xml_unpack_t& x, const cd::string& d, cd::shared_ptr<T>& a)
    {xml_unpack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_xml_unpack<std::auto_ptr<T> >
  {
    void operator()(cd::xml_unpack_t& x, const cd::string& d, std::auto_ptr<T>& a)
    {xml_unpack_smart_ptr(x,d,a);}
  };


#if defined(__cplusplus) && __cplusplus >= 201103L
  template <class T>
  struct access_xml_unpack<std::unique_ptr<T> >
  {
    void operator()(cd::xml_unpack_t& x, const cd::string& d, std::unique_ptr<T>& a)
    {xml_unpack_smart_ptr(x,d,a);}
  };
#endif
}
 

#endif
#endif
