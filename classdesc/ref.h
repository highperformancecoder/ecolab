/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief Reference counted smart pointer classes
*/

#ifndef REF_H
#define REF_H
#include "pack_base.h"
#include "pack_graph.h"
#include "function.h"
#include <algorithm>

namespace classdesc
{

  /**
     A non-polymorphic smart pointer class
  */
  template <class T>
  class ref
  {
    struct Payload: public T
    {
      int count;
      Payload(): count(1) {}
      Payload(const T& x): T(x), count(1) {}
    };
    Payload* item;
    void refdec() {if (item){item->count--; if (item->count==0) delete item;}}
    void asg(const ref& x) 
    {
      if (x.item)
        {item=x.item; item->count++;}
      else
        item=NULL;
    }
    void newitem() {item=new Payload;}
    void newitem(const T& x) {item=new Payload(x);}
    template <class U> bool operator==(const U&) const;
  public:
    ref(): item(NULL) {}  /* unitialised refs don't consume space */
    ref(const ref& x) {asg(x);}
    ref(const T& x) {newitem(x);}
    ref& operator=(const ref& x)  { if (x.item!=item) {refdec(); asg(x);} return *this;}
    ref& operator=(const T& x) {refdec(); newitem(x); return *this;}  
    ~ref() {refdec();}
    /// dereference - creates default object if null
    T* operator->() {if (!item) newitem(); return (T*)item;}
    template <class M>
    functional::bound_method<T,M> operator->*(M& m) {
      return functional::bound_method<T,M>(**this, m);
    }
    template <class M>
    functional::bound_method<T,M> operator->*(M& m) const {
      return functional::bound_method<T,M>(**this, m);
    }
    /// dereference - creates default object if null
    T& operator*() {if (!item) newitem(); return *(T*)item;}
    /// dereference - throws in debug mode if null
    const T* operator->() const {assert(item); return (T*)item;}
    /// dereference - throws in debug mode if null
    const T& operator*() const {assert(item); return *(T*)item;}
    /// make reference null
    void nullify() { refdec(); item=NULL;}
    /// true if reference is null
    bool nullref() const { return item==NULL;} 
    /// equivalent to nullref
    operator bool () const {return !nullref();}
    /// return the payloads reference count
    int refCount() const {if (item) {return item->count;} else return 0;}
    bool operator==(const ref& x) const {return x.item==item;}
    bool operator==(const T* x) const {return x==item;} 
    bool operator==(const T& x) const {return x==*item;} 
    template <class U>
    bool operator!=(const U& x) const {return !operator==(x);}
    /* used for maps and sets - needed for unpack */
    bool operator<(const ref& x) const {return item<x.item;}
    void swap(ref& x) {Payload*tmp=x.item; x.item=item; item=tmp;}
  };

  /**
     Default allocator class for \c ref
  */
  template <class T> 
  struct Alloc<classdesc::ref<T> >
  {
    /* operator*() creates new space, if x is NULL */
    /* there is no need to store references into alloced, as ref 
       will handle its own cleanup */
    void operator()(pack_t& buf, ref<T>& x) {x=T();}
  };

}

#ifdef _CLASSDESC
#pragma omit pack classdesc::ref
#pragma omit unpack classdesc::ref
#pragma omit isa classdesc::ref
#endif

namespace classdesc_access
{
  template <class T>
  struct access_pack<classdesc::ref<T> >
  {
    template <class U>
    void operator()(classdesc::pack_t& t, const classdesc::string& d, U& a) 
    {pack_graph(t,a);}
  };

  template <class T>
  struct access_unpack<classdesc::ref<T> >
  {
    template <class U>
    void operator()(classdesc::pack_t& t, const classdesc::string& d, U& a) 
    {unpack_graph(t,a);}
  };
}

namespace std
{
  template <class T> void swap(classdesc::ref<T>& x,classdesc::ref<T>& y) {x.swap(y);}
}

#endif
