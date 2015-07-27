/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
   \brief textual representation descriptor
*/

#ifndef DUMP_BASE_H
#define DUMP_BASE_H

#include <iostream>
#include <sstream>
#include <iomanip>

#include "classdesc.h"
#include <stdarg.h>

namespace classdesc
{
  /**
     A standard stream object that outputs a textual representation of
     complex objects.
  */
  class dump_t: public std::ostream 
  {
  public:
    dump_t(): std::ostream(std::cout.rdbuf()) {}
    dump_t(std::ostream& o): std::ostream(o.rdbuf()) {}
  };

  /// forward declare generic dump operation
  template <class T> void dump(dump_t& o, const string& d, const T& a);

  /// utility for using existing ostreams directly
  template <class T> void dump(std::ostream& o, const string& d, const T& a)
  {dump_t t(o); classdesc::dump(t,d,a);}

  inline int format(std::ostream& o, const string& d)
  {
    const char* tail=d.c_str();
    int ndots=0;
    for (const char* c=tail; *c!='\0'; c++)
      if (*c=='.') 
        {
          ndots++;
          o<<' ';
          tail=c+1;
        }
    o<<tail<<":";
    return ndots;
  }

  //basic C types
  template <class T>
  void dump_basic(dump_t& o, const string& d, T& a)
  {
    format(o,d);
    o<<a<<std::endl;
  }

  template <class B>
  typename enable_if< is_fundamental<B>, void >::T
  dumpp(dump_t& o, const string& d, B& a) {dump_basic(o,d,a);}

  inline void dumpp(dump_t& o, const string& d, const char *& a) {dump_basic(o,d,a);}
  template <class W>
  void dumpp(dump_t& o, const string& d, std::basic_string<W>& a) 
  {dump_basic(o,d,a);}

  /// handle maps, whose value_type is a pair
  template <class A, class B>
  typename enable_if< And<is_fundamental<A>,is_fundamental<B> >, void>::T
  dump(dump_t& o, const string& d, std::pair<A,B>& a, dummy<0> dum=0)
  {o << "["<< a.first<<"]="<<a.second;}

  template <class A, class B>
  typename enable_if< And<is_fundamental<A>,Not<is_fundamental<B> > >, void>::T
  dump(dump_t& o, const string& d, std::pair<A,B>& a, dummy<1> dum=0)
  {
    o << "["<< a.first<<"]=";
    dump(o,d,a.second);
  }

  template <class A, class B>
  typename enable_if< Not<is_fundamental<A> >, void>::T
  dump(dump_t& o, const string& d, std::pair<A,B>& a, dummy<2> dum=0)
  {
    dump(o,string("[.")+d+"]=",a.first);
    dump(o,d,a.second);
  }

  template <class I>
  typename enable_if< is_fundamental<
    typename std::iterator_traits<I>::value_type>, 
                      void>::T
  dump_container(dump_t& o, const string& d, I begin, 
                 const I& end, dummy<0> dum=0)
  {
    int tab = format(o,d);
    for (; begin!=end; begin++)
      o << std::setw(tab) << "" << *begin;
    o<<std::endl;
  }

  template <class I>
  typename enable_if< Not<is_fundamental<
    typename std::iterator_traits<I>::value_type> >, 
                      void>::T
  dump_container(dump_t& o, const string& d, I begin, 
                 const I& end, dummy<1> dum=0)
  {
     int tab = format(o,d);
     o<<std::endl;
     for (size_t i=0; begin!=end; i++,begin++)
       {
         o<<std::setw(tab+2)<<"{"<<std::endl;
         std::ostringstream idx;
         idx<<i;
         dump(o,d+"["+idx.str().c_str()+"]",*begin);
         o<<std::setw(tab+2)<<"}"<<std::endl;
       }
  }

  template <class C>
  typename enable_if<is_container<C>, void>::T
  dump(dump_t& o, const string& d, C& a, dummy<0> dum=0)
  {dump_container(o,d,a.begin(),a.end());}

  /// ignore member functions
  template <class C, class M>
  void dump(dump_t& o, const string& d, C& a, M m) {}
  
  ///handle enums
  template <class T>
  void dump(dump_t& o, const string& d, Enum_handle<T> arg)
  {dump_basic(o,d,arg);}

  ///handle arrays
  template <class T>
  void dump(dump_t& o, const string& d, is_array ia, T& arg,
            int dims, size_t ncopies, ...)
  {
    va_list ap;
    va_start(ap,ncopies);
    for (int i=1; i<dims; i++) ncopies*=va_arg(ap,int); //assume that 2 and higher D arrays dimensions are int
    va_end(ap);
    if (ncopies) dump_container(o,d,&arg, &arg+ncopies);
  }

  /// const static support
  template <class T>
  void dump(dump_t& o, const string& d, is_const_static i, T arg)
  {dump(o,d,arg);}

  template <class T, class U>
  void dump(dump_t& o, const string& d, is_const_static i, const T&, U) {}

   template <class T>
   void dump(dump_t& o, const string& d, Exclude<T>&) {}

  // shared_ptr support - polymorphism not supported
  template <class T>
  void dump(dump_t& o, const string& d, shared_ptr<T>& a) 
  {
    if (a)
      dump(o,d,*a);
    else
      dump(o,d,string("null"));
  }

}

namespace classdesc_access
{
  template <class T> struct access_dump;
}

using classdesc::dump;
using classdesc::dumpp;

namespace classdesc
{
  //TODO maybe we could indicate inheritance more smartly than this?
  template <class T>
  void dump_onbase(dump_t& x,const string& d,T& a)
  {::dump(x,d,a);}
}
using classdesc::dump_onbase;

#endif
