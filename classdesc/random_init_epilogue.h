/*
  @copyright Russell Standish 2000-2014
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef RANDOM_INIT_EPILOGUE_H
#define RANDOM_INIT_EPILOGUE_H
#include "classdesc.h"

namespace classdesc_access
{
  namespace cd=classdesc;
  template <class T>
  struct access_random_init<const T>
  {
    void operator()(classdesc::random_init_t&,const classdesc::string&,const T&a) 
    {/* nothing to do */}
  };

  template<class T> typename
  cd::enable_if<cd::is_default_constructible<typename T::element_type>, void>::T
  random_init_smart_ptr(cd::random_init_t& r, const cd::string& d, 
                        T& a, cd::dummy<0> dum=0)
  {
    a.reset(new typename T::element_type);
    random_init(r,d,*a);
  }

  template<class T> typename
  cd::enable_if<
    cd::Not<cd::is_default_constructible<typename T::element_type> >, void>::T
  random_init_smart_ptr(cd::random_init_t& r, const cd::string& d, 
                        T& a, cd::dummy<1> dum=0)
  {
    if (a.get())
      random_init(r,d,*a);
  }

  template <class T>
  struct access_random_init<cd::shared_ptr<T> >
  {
    void operator()(cd::random_init_t& x, const cd::string& d, 
                    cd::shared_ptr<T>& a)
    {random_init_smart_ptr(x,d,a);}
  };

  template <class T>
  struct access_random_init<std::auto_ptr<T> >
  {
    void operator()(cd::random_init_t& x, const cd::string& d, 
                    std::auto_ptr<T>& a)
    {random_init_smart_ptr(x,d,a);}
  };

#if defined(__cplusplus) && __cplusplus >= 201103L
  template <class T, class D>
  struct access_random_init<std::unique_ptr<T,D> >
  {
    void operator()(cd::random_init_t& x, const cd::string& d, 
                    std::unique_ptr<T,D>& a)
    {json_pack_smart_ptr(x,d,a);}
  };

#endif
  

  // support for polymorphic types, if loaded
#ifdef NEW_POLY_H
  template <class T> struct access_random_init<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::random_init_t> {};
  template <class T, class B> struct access_random_init<cd::Poly<T,B> >
  {
    template <class U>
    void operator()(cd::random_init_t& t, const cd::string& d, U& a)
    {
      random_init(t,d,cd::base_cast<B>::cast(a));
    }
  };
#endif

#ifdef POLYPACKBASE_H
  template <> struct access_random_init<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::random_init_t> {};
  template <class T> struct access_random_init<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::random_init_t> {};
#endif

#ifdef POLYJSONBASE_H
  template <> struct access_random_init<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::random_init_t> {};
  template <class T> struct access_random_init<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::random_init_t> {};
#endif

#ifdef POLYXMLBASE_H
  template <> struct access_random_init<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::random_init_t> {};
  template <class T> struct access_random_init<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::random_init_t> {};
#endif

  

}

namespace classdesc
{
  template <class T>
  struct AllOtherRandomInitTypes:
    public Not< Or< Or< Or<is_arithmetic<T>,is_string<T> >, is_sequence<T> >, 
                    is_associative_container<T> > >
  {};

  template <class T> typename 
  enable_if<AllOtherRandomInitTypes<T>, void >::T
  random_initp(random_init_t& r,const string& d,T& a, dummy<3> dum=0)
  {classdesc_access::access_random_init<T>()(r,d,a);}

  // by default, do nothing
  template <class T> void random_init_normalise(T&) {}

  template <class T>
  void random_init(random_init_t& r,const string& d,T& a)
  {
    random_initp(r,d,a);
    random_init_normalise(a);
  }
}
#endif
