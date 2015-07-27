/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef OBJECT_H
#define OBJECT_H
#include "classdesc.h"
#include "pack_base.h"

#include <limits>

namespace classdesc
{
  struct object;
  typedef std::vector<shared_ptr<object> > Factory;
  inline Factory& factory()
  {
    // ensure factory is initialised on first use
    static Factory f;
    return f;
  }

  /** abstract base class for a polymorphic heirarchy */
  struct object
  {
    typedef int TypeID;
    virtual TypeID type() const=0;
    static object* create(TypeID);
    virtual object* clone() const=0;
    object* cloneT() const {return clone();}
    virtual void pack(pack_t& b) const=0;
    virtual void unpack(pack_t& b)=0;
    virtual ~object() {}
  };

  inline void register_with_factory(shared_ptr<object> o)
  {
    if (o->type()>=int(factory().size()))
      factory().resize(o->type()+1);
    factory()[o->type()]=o;;
  }

  // it would be nice for this to be a static method of object, but it
  // appears static methods cannot be inlined
  inline object* object::create(object::TypeID t) {// factory
    assert(t<object::TypeID(factory().size()) && t>=0);
    return factory()[t]->clone();
  }

  template <class T>
  struct Register
  {
    Register() {register_with_factory(shared_ptr<object>(new T));}
  };

  /** template for handling polymorphic heirarchy, saves having to
    explicitly declare all the virtual functions. Uses the curiously
    recurring template pattern. Declare a new type \c foo, derived
    from \c bar having type identifier \c t as 

    \code 
    class foo: public
    Object<foo,t,bar> 
    {...
    \endcode

    NB. This class is unsuitable as a base of an abstract class. Use
    \c object for that purpose instead.
  
  */
  template <class This, class Base=object>
  struct Object: virtual public Base 
  {
    /// warning, this method should not be called before main() in any
    /// other module than the one declaring classdesc::factory
    virtual typename Base::TypeID type() const {
      static typename Base::TypeID t=-1;
      if (t==-1) 
        {
          t=typename Base::TypeID(factory().size());
          register_with_factory(shared_ptr<object>(clone()));
        }
      return t;
    }
    virtual object* clone() const {
      return new This(*dynamic_cast<const This*>(this));}
    /// same as clone(), but returning fully typed pointer
    This *cloneT() const {return dynamic_cast<This*>(clone());} 
    virtual void pack(pack_t& b) const {
      ::pack(b,"",*dynamic_cast<const This*>(this));}
    virtual void unpack(pack_t& b) {
      ::unpack(b,"",*dynamic_cast<This*>(this));}
  };

  template <class T>
  struct pack_supported<classdesc::shared_ptr<T> >
  {static const bool value=is_base_of<object,T>::value;};

}

/// shared pointer serialisation support
template <class T> typename
classdesc::enable_if<classdesc::is_base_of<classdesc::object,T>, void>::T
pack(classdesc::pack_t& b, const classdesc::string& d, const classdesc::shared_ptr<T>& a)
{
  if (a)
    {
      b<<(a->type()+1);
      a->pack(b);
    }
  else
    b<<classdesc::object::TypeID(0);
}

template <class T> typename
classdesc::enable_if<classdesc::is_base_of<classdesc::object,T>, void>::T
unpack(classdesc::unpack_t& b, const classdesc::string& d, classdesc::shared_ptr<T>& a)
{
  classdesc::object::TypeID t;
  b>>t;
  if (t)
    {
      a.reset(classdesc::object::create(t-1));
      a->unpack(b);
    }
}

template <class T> typename
classdesc::enable_if<classdesc::is_base_of<classdesc::object,T>, void>::T
unpack(classdesc::unpack_t& b, const classdesc::string& d, const classdesc::shared_ptr<T>& a)
{
  classdesc::object::TypeID t;
  b>>t;
  if (t)
    {
      std::auto_ptr<T> a(classdesc::object::create(t-1));
      a->unpack(b);
    }
}


namespace classdesc_access
{
  namespace cd=classdesc;

  template <> struct access_pack<classdesc::object>: public cd::NullDescriptor<cd::pack_t>{};
  template <> struct access_unpack<classdesc::object>: public cd::NullDescriptor<cd::pack_t>{};

  template <class T, class B> 
  struct access_pack<classdesc::Object<T, B> >
  {
    template <class U>
    void operator()(cd::pack_t& b, const cd::string& d, U& a) 
    {pack(b,d,cd::base_cast<B>::cast(a));}
  };

  template <class T, class B> 
  struct access_unpack<classdesc::Object<T, B> >
  {
    template <class U>
    void operator()(cd::unpack_t& b, const cd::string& d, U& a) 
    {unpack(b,d,cd::base_cast<B>::cast(a));}
  };

}

#ifdef _CLASSDESC
#pragma omit pack classdesc::object
#pragma omit unpack classdesc::object
#pragma omit pack classdesc::Object
#pragma omit unpack classdesc::Object
#endif
#endif
