/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef JSON_PACK_EPILOGUE_H
#define JSON_PACK_EPILOGUE_H
#include "classdesc.h"
#include <sstream>

namespace classdesc
{
  template <class T>
  struct AllOtherJsonPackpTypes:
    public Not< Or< Or< Or<is_fundamental<T>,is_string<T> >, is_sequence<T> >, 
                    is_associative_container<T> > >
  {};

  template <class T> typename 
  enable_if< AllOtherJsonPackpTypes<T>, void >::T
  json_packp(json_pack_t& o, const string& d, T& a)
  {
    if (tail(d)!="")
      {
        //create the object, if it doesn't already exist
        try
          {
            json_spirit::mValue& parent=json_find(o,head(d));
            if (parent.type()!=json_spirit::obj_type)
              throw json_pack_error("trying to create object %s in non-object",
                                    d.c_str());
            json_spirit::mObject::iterator member=parent.get_obj().find(tail(d));
            if (member==parent.get_obj().end())
              parent.get_obj().insert(make_pair(tail(d), json_spirit::mObject()));
          }
         catch (json_pack_error&)
          {
            // only throw if this flag is set
            if (o.throw_on_error) throw; 
          }
      }
    classdesc_access::access_json_pack<T>()(o,d,a);
  }

  template <class T> typename
  enable_if< AllOtherJsonPackpTypes<T>, void >::T
  json_unpackp(json_pack_t& o, const string& d, T& a, dummy<3> dum=0)
  {classdesc_access::access_json_unpack<T>()(o,d,a);}

  template <class T> void json_pack(json_pack_t& o, const string& d, T& a)
  {json_packp(o,d,a);}

  template <class T> void json_unpack(json_unpack_t& o, const string& d, T& a)
  {json_unpackp(o,d,a);}

}

namespace classdesc_access
{
  namespace cd=classdesc;
  // fall through to streaming operators
  template <class T>
  struct access_json_pack
  {
  public:
    void operator()(cd::json_pack_t& b, const cd::string& d, T& a)
    {
      std::ostringstream o;
      o<<a;
      b<<o.str();
    }
  };

  template <class T>
  struct access_json_unpack
  {
  public:
    void operator()(cd::json_unpack_t& b, const cd::string& d, 
                    T& a)
    {
      std::string s;
      b>>s;
      std::istringstream i(s);
      i>>a;
    }
  };

  // support for polymorphic types, if loaded
#ifdef NEW_POLY_H
  template <class T> struct access_json_pack<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <class T> struct access_json_unpack<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
  template <class T, class B> struct access_json_pack<cd::Poly<T,B> >
  {
    template <class U>
    void operator()(cd::json_pack_t& t, const cd::string& d, U& a)
    {
      json_pack(t,d,cd::base_cast<B>::cast(a));
    }
  };
  template <class T, class B> struct access_json_unpack<cd::Poly<T,B> > 
  {
    template <class U>
    void operator()(cd::json_pack_t& t, const cd::string& d, U& a)
    {
      json_unpack(t,d,cd::base_cast<B>::cast(a));
    }
  };
#endif

#ifdef POLYPACKBASE_H
  template <> struct access_json_pack<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <> struct access_json_unpack<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
  template <class T> struct access_json_pack<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <class T> struct access_json_unpack<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
#endif

#ifdef POLYJSONBASE_H
  template <> struct access_json_pack<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <> struct access_json_unpack<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
  template <class T> struct access_json_pack<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <class T> struct access_json_unpack<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
#endif

#ifdef POLYXMLBASE_H
  template <> struct access_json_pack<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <> struct access_json_unpack<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
  template <class T> struct access_json_pack<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::json_pack_t> {};
  template <class T> struct access_json_unpack<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::json_unpack_t> {};
#endif

}  

#include "polyAccessJsonPack.h"
#endif
