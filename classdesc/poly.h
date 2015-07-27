/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLY_H
#define POLY_H
// functionality in this file is completely deprecated in favour of
// the Poly class in Poly.h
#include "object.h"
#include "ref.h"

namespace classdesc
{

  /** empty concrete object type */
  class Eobject: public Object<Eobject> {};

  /**
     A simple RTTI system that can be used, reimplemented and passed to polyref
  */
  template <class T=Eobject>
  struct SimpleTypeTable
  {
    static std::vector<T*> data;
  public:
    T& operator[](std::size_t i) {return *data[i];}
    void register_type(const T* t) {
      if (t->type()+1>int(data.size()))
        data.resize(t->type()+1);
      if (!data[t->type()])
        data[t->type()]=dynamic_cast<T*>(t->clone());
    }
  };

  template <class T> std::vector<T*> SimpleTypeTable<T>::data;

  /** polymorphic smart pointer class - copies are deep */
  template <class T=Eobject, class TT=SimpleTypeTable<T> >
  class poly
  {
    T* item;
    // force a compile error if these are used
    template <class U, class UU> bool operator==(const poly<U,UU>&) const;
    template <class U, class UU> bool operator!=(const poly<U,UU>&) const;
    void asg(const poly& x) {
      if (x) item=dynamic_cast<T*>(x->clone()); else item=NULL;
    }
  public:
    TT TypeTable;
    poly(): item(NULL) {}
    poly(const poly& x) {asg(x);}
    poly(const T& x) {item=dynamic_cast<T*>(x.clone()); }
    poly& operator=(const poly& x)  {delete item; asg(x); return *this;}
    poly& operator=(const T& x) {
      delete item; item=dynamic_cast<T*>(x.clone()); return *this;}  
    ~poly() {delete item;}

    //@{ 
    /// Target object initialisation 
    /** Initialise target object to type \a U. 0, 1 and 2 argument
        constructors available. Use 1 arg constructor and copy
        construction for arbitrary initialisaton.
     */
    template <class U> poly addObject() {
      delete item; item=new U; TypeTable.register_type(item); 
      return *this;}
    template <class U, class A> poly addObject(A x) {
      delete item; item=new U(x); TypeTable.register_type(item); 
      return *this;}
    template <class U, class A1, class A2> poly addObject(A1 x1, A2 x2) {
      delete item; item=new U(x1,x2); TypeTable.register_type(item); 
      return *this;}
    //@}
    T* operator->() {assert(item); return item;}
    T& operator*() {assert(item); return *item;}
    const T* operator->() const {assert(item); return item;}
    const T& operator*() const {assert(item); return *item;}
    
    //@{
    /// cast target to type \a U. Thows std::bad_cast if impossible.
    template <class U> U& cast() {return dynamic_cast<U&>(*item);}
    template <class U> const U& cast() const {return dynamic_cast<U&>(*item);}
    //@}

    void swap(poly& x) {std::swap(item,x.item);}

    bool operator==(const poly& x) const {return x.item==item;}
    bool operator!=(const poly& x) const {return x.item!=item;};
    operator bool() const {return item!=NULL;}
  };

  /** reference counted polymorphic smart pointer class - copies are shallow */
  template <class T=Eobject, class TT=SimpleTypeTable<T> >
  class polyref: public classdesc::ref<classdesc::poly<T,TT> >
  {
    typedef ref<poly<T,TT> > super;
    poly<T,TT>& polyObj() {return super::operator*();}
    const poly<T,TT>& polyObj() const {return super::operator*();}
    // prevent problems with accidental bool conversions
    template <class U> bool operator==(const U&) const;
  public:
    // delegate poly specific methods
    //@{
    /// add object of type U
    template <class U> polyref addObject() {
      polyObj().template addObject<U>(); 
      return *this;
    }
    template <class U, class A> polyref addObject(A x) {
      polyObj().template addObject<U,A>(x); 
      return *this;}
    template <class U, class A1, class A2> polyref addObject(A1 x1, A2 x2) {
      polyObj().template addObject<U,A1,A2>(x1,x2); 
      return *this;}
    //@}

    T* operator->() {return &*polyObj();}
    T& operator*() {return *polyObj();}
    const T* operator->() const {return &*polyObj();}
    const T& operator*() const {return *polyObj();}
    
    bool operator==(const polyref& x) {
      return super::operator==(static_cast<super&>(x));
    }
    template <class U>
    bool operator!=(const U& x) const {return !operator==(x);}

    //@{
    /// cast target to type U. Throws if not possible
    template <class U> U& cast() {return polyObj().template cast<U>();}
    template <class U> const U& cast() const {return polyObj().template cast<U>();}
    //@}
    operator bool () const {return !super::nullref() && bool(polyObj());}
  };

#ifdef _CLASSDESC
#pragma omit pack classdesc::poly
#pragma omit unpack classdesc::poly
#pragma omit isa classdesc::poly
#pragma omit pack classdesc::object
#pragma omit unpack classdesc::object
#pragma omit javaClass classdesc::object
#pragma omit pack classdesc::Object
#pragma omit unpack classdesc::Object
#pragma omit javaClass classdesc::Object
#pragma omit dump classdesc::SimpleTypeTable
#pragma omit dump classdesc::poly
#pragma omit javaClass classdesc::poly
#endif
}

namespace classdesc_access
{
  template <class T, class TT>
  struct access_pack<classdesc::poly<T,TT> >
  {
    template <class U>
    void operator()(classdesc::pack_t& t, const classdesc::string& d, U& a) 
    {
      t<<bool(a);
      if (a)
        {
          t<<a->type();
          a->pack(t);
        }
    }
  };
    
  template <class T, class TT>
  struct access_unpack<classdesc::poly<T,TT> >
  {
    template <class U>
    void operator()(classdesc::pack_t& t, const classdesc::string& d, U& a) 
    {
      bool valid; t>>valid;
      if (valid)
        {
          typename T::TypeID type;
          t>>type;
          if (!a || type!=a->type())
            a=a.TypeTable[type];
          a->unpack(t);
        }
    }
  };

}

namespace classdesc
{
//TODO is this really what we want to do with dump of polys?
  class dump_t;
  template <class T> void dump(dump_t&, const string&, const T&);

  template <class T, class TT>
  void dump(dump_t& t, const string& d, const poly<T,TT>& a) {}
}
using classdesc::dump;

/**
   efficient swap implementations
*/
namespace std
{
  template <class T, class TT> void swap(classdesc::poly<T,TT>& x,classdesc::poly<T,TT>& y) {x.swap(y);}
}
#endif
