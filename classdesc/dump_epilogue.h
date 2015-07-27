/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef DUMP_EPILOGUE_H
#define DUMP_EPILOGUE_H


#include <iostream>
namespace classdesc
{
  template <class B> 
  typename enable_if< Not<is_fundamental<B> >, void >::T
  dumpp(dump_t& t, const string& d, B& a)
  {
    classdesc_access::access_dump<B>()(t,d,a);
  }

  template <class T>
  typename enable_if<is_container<T>, bool>::T
  is_basic_container() {
    return is_fundamental<typename T::value_type>::value;
  }

  template <class T>
  typename enable_if<Not<is_container<T> >, bool>::T
  is_basic_container() {return false;}

  // only format for complex types
  template <class T>
  struct Format
  {
    int tab;
    std::ostream& o;
    Format(std::ostream& o, const string& d): tab(-1), o(o) 
    {
      if (!is_fundamental<T>::value && !is_string<T>::value
          && !is_basic_container<T>() )
        {
          tab=format(o,d); 
          o<<"{"<<std::endl;
        }
    }
    ~Format() 
    {
      if (tab>=0)
        o<<std::setw(tab+2)<<"}"<<std::endl;
    }
  };
      
  template <class T> void dump(classdesc::dump_t& o, 
                               const classdesc::string& d, const T& a)
  {
    Format<T> f(o,d);
    dumpp(o,d,const_cast<T&>(a));
  }

//  template <class T>
//  std::ostream& operator<<(std::ostream& o,const T& a)
//  {dump(&o,"",a);}
}

namespace classdesc_access
{
  namespace cd=classdesc;
  // support for polymorphic types, if loaded
#ifdef NEW_POLY_H
  template <class T> struct access_dump<cd::PolyBase<T> >: 
    public cd::NullDescriptor<cd::dump_t> {};
  template <class T, class B> struct access_dump<cd::Poly<T,B> >: 
    public cd::NullDescriptor<cd::dump_t> {};
#endif

#ifdef POLYPACKBASE_H
  template <> struct access_dump<cd::PolyPackBase>: 
    public cd::NullDescriptor<cd::dump_t> {};
  template <class T> struct access_dump<cd::PolyPack<T> >: 
    public cd::NullDescriptor<cd::dump_t> {};
#endif

#ifdef POLYJSONBASE_H
  template <> struct access_dump<cd::PolyJsonBase>: 
    public cd::NullDescriptor<cd::dump_t> {};
  template <class T> struct access_dump<cd::PolyJson<T> >: 
    public cd::NullDescriptor<cd::dump_t> {};
#endif

#ifdef POLYXMLBASE_H
  template <> struct access_dump<cd::PolyXMLBase>: 
    public cd::NullDescriptor<cd::dump_t> {};
  template <class T> struct access_dump<cd::PolyXML<T> >: 
    public cd::NullDescriptor<cd::dump_t> {};
#endif

#ifdef FACTORY_H
  template <class T, class U> struct access_dump<cd::Factory<T,U> > 
  {
    void operator()(cd::dump_t& b,const cd::string& d, cd::Factory<T,U>& a)
    {
      dump(b,d,a.fmap);
    }
  };
#endif
}

#endif
