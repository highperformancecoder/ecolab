/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef NEW_POLY_H
#define NEW_POLY_H
#include <classdesc.h>

namespace classdesc
{
  /// used for metaprogrammatically distinguishing between polymorphic
  /// and non-polymorphic types
  struct PolyBaseMarker {};

  /// base class for polymorphic types. T is a type enumerator class
  template <class T>
  struct PolyBase: public PolyBaseMarker
  {
    typedef T Type;
    virtual Type type() const=0;
    virtual PolyBase* clone() const=0;
    /// cloneT is more user friendly way of getting clone to return the
    /// correct type. Returns NULL if \a U is invalid
    template <class U>
    U* cloneT() const {
      std::auto_ptr<PolyBase> p(clone());
      U* t=dynamic_cast<U*>(p.get());
      if (t)
        p.release();
      return t;
    }
    virtual ~PolyBase() {}
  };


  /// utility class for building the derived types in a polymorphic
  /// heirarchy. \a Base is the base of the heirarchy. 
  /**
     A possible use is as follows:

     class MyBase: public PolyBase<int>
     {
        static MyBase* create(int); // factory method
     };

     template <int t>
     class MyClass: public Poly<MyClass, MyBase>
     {
        public:
          int type() const {return t;}
     };

     // see also \c Factory class for another way of doing this
     static MyBase* MyBase::create(int t)
     {
        switch (t)
        {
          case 0: return new MyClass<0>;
          case 1: return new MyClass<1>;
          case 2: return new MyClass<2>;
          default: 
            throw std::runtime_error("unknown class construction requested");
        }
     }

     See also descriptor specific poly classes, such as polypack.h
  */

  template <class T, class Base>
  struct Poly: virtual public Base
  {
    /// clone has to return a Poly* to satisfy covariance
    Poly* clone() const {return new T(*static_cast<const T*>(this));}
  };

  template <class T> struct tn<PolyBase<T> >
  {
    static std::string name()
    {return "classdesc::PolyBase<"+typeName<T>()+">";}
  };
  template <class T, class Base> struct tn<Poly<T,Base> >
  {
    static std::string name()
    {return "classdesc::Poly<"+typeName<T>()+","+typeName<Base>()+">";}
  };


}

#endif
