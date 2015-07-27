/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLYPACKBASE_H
#define POLYPACKBASE_H

////#include "polyBase.h"
#include "pack_base.h"

namespace classdesc
{
  /// interface for applying pack descriptors to polymorphic objects
  struct PolyPackBase
  {
    virtual void pack(pack_t&, const string&) const=0;
    virtual void unpack(unpack_t&, const string&)=0;
    virtual ~PolyPackBase() {}
  };

  /// utility class for defining pack descriptors for polymorphic types
  /**
     Use as
     class MyBase: public PolyBase<int>
     {
        static MyBase* create(int); // factory method
     };

     template <int t>
     class MyClass: public Poly<MyClass, MyBase>, public PolyPack<MyClass, int>
     {
        public:
          int type() const {return t;}
     };

     Note use of "curiously recurring template pattern", and multiple
     inheritance to implement mixins.
  */

  template <class T>
  struct PolyPack: virtual public PolyPackBase
  {
    void pack(pack_t& x, const string& d) const
    {::pack(x,d,static_cast<const T&>(*this));}
      
    void unpack(unpack_t& x, const string& d)
    {::unpack(x,d,static_cast<T&>(*this));}

  };

  template <> struct tn<PolyPackBase>
  {
    static std::string name()
    {return "classdesc::PolyPackBase";}
  };
  template <class T> struct tn<PolyPack<T> >
  {
    static std::string name()
    {return "classdesc::PolyPack<"+typeName<T>()+">";}
  };
}
#endif
      
