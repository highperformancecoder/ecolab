/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLY_ACCESS_JSON_PACK_H
#define POLY_ACCESS_JSON_PACK_H

namespace classdesc
{
  struct ResetThrowOnErrorOnExit
  {
    json_pack_t& x;
    bool prev;
    ResetThrowOnErrorOnExit(json_pack_t& x): x(x), prev(x.throw_on_error) {}
    ~ResetThrowOnErrorOnExit() {x.throw_on_error=prev;}
  };

#ifdef POLYJSONBASE_H
  // polymorphic version
  template <class T>
  typename enable_if<is_base_of<PolyJsonBase, typename T::element_type>, void>::T
  json_pack_smart_ptr(json_pack_t& x, const string& d, T& a, 
                       dummy<0> dum=0)
  {
    if (a.get())
      {
        ::json_pack(x,d+".type",a->type());
        a->json_pack(x,d);
      }
  }

  // non polymorphic version
  template <class T>
  typename enable_if<Not<is_base_of<PolyJsonBase, typename T::element_type> >, void>::T
#else
  template <class T>
  void
#endif
  json_pack_smart_ptr(json_pack_t& x, const string& d, T& a, 
                       dummy<1> dum=0)
  {
    if (a.get())
      ::json_pack(x,d,*a);
  }

#ifdef POLYJSONBASE_H
    // polymorphic version
  template <class T>
  typename enable_if<is_base_of<PolyJsonBase, typename T::element_type>, 
                     void>::T
  json_unpack_smart_ptr(json_unpack_t& x, const string& d, T& a,
                         dummy<0> dum=0)
  {
    if (x.type()==json_spirit::obj_type && x.get_obj().count("type"))
      {
        typename T::element_type::Type type;
        ::json_unpack(x,d+".type",type);
        a.reset(T::element_type::create(type));
        a->json_unpack(x,d);
      }
    else
      a.reset();
  }

  // non polymorphic version
  template<class T>
  typename enable_if<Not<is_base_of<PolyJsonBase, typename T::element_type> >, 
                     void>::T
#else
  template<class T>
  void
#endif
  json_unpack_smart_ptr(json_pack_t& x, const string& d, T& a,
                           dummy<1> dum=0)
  {
    ResetThrowOnErrorOnExit r(x);
    a.reset(new typename T::element_type);
    try
      {
        ::json_unpack(x,d,*a);
      }
    catch (const std::exception&)
        {
          a.reset(); // data wasn't there
        }
  }
  

}

namespace classdesc_access
{
  namespace cd=classdesc;
  template <class T>
  struct access_json_pack<cd::shared_ptr<T> >
  {
    void operator()(cd::json_pack_t& x, const cd::string& d, 
                    cd::shared_ptr<T>& a)
    {json_pack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_json_unpack<cd::shared_ptr<T> >
  {
    void operator()(cd::json_unpack_t& x, const cd::string& d, 
                    cd::shared_ptr<T>& a)
    {json_unpack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_json_pack<std::auto_ptr<T> >
  {
    void operator()(cd::json_pack_t& x, const cd::string& d, 
                    std::auto_ptr<T>& a)
    {json_pack_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_json_unpack<std::auto_ptr<T> >
  {
    void operator()(cd::json_unpack_t& x, const cd::string& d, 
                    std::auto_ptr<T>& a)
    {json_unpack_smart_ptr(x,d,a);}
  };

#if defined(__cplusplus) && __cplusplus >= 201103L
  template <class T, class D>
  struct access_json_pack<std::unique_ptr<T,D> >
  {
    void operator()(cd::json_pack_t& x, const cd::string& d, 
                    std::unique_ptr<T,D>& a)
    {json_pack_smart_ptr(x,d,a);}
  };

  template <class T, class D>
  struct access_json_unpack<std::unique_ptr<T,D> >
  {
    void operator()(cd::json_unpack_t& x, const cd::string& d, 
                    std::unique_ptr<T,D>& a)
    {json_unpack_smart_ptr(x,d,a);}
  };
#endif

}

#endif
