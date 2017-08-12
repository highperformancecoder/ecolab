/*
  @copyright Russell Standish 2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ACCESSOR_H
#define ACCESSOR_H
#include "pack_base.h"
#if defined(__cplusplus) && __cplusplus>=201103L
#include <functional>
#endif

namespace ecolab
{
#if defined(__cplusplus) && __cplusplus>=201103L
  using std::function;
#else
  template <class F>
  struct function {};
#endif

  /**
     Allows TCL access to overloaded getter/setter methods, which
     normally is not available due to the overload restrictions in
     classdesc
   */
  template <class T, class Getter=function<T()>, 
            class Setter=function<T(const T&)> >
  struct Accessor
  {
    Getter g;
    Setter s;
    Accessor(const Getter& g, const Setter& s): g(g), s(s) {}
    T operator()() const {return g();}
    T operator()(const T& x) const {return s(x);}
    // assignment and copying don't really make sense
    void operator=(const Accessor&) {}
  private:
    // there is no way to give a meaningful copy constructor, as this
    // object needs to contain a reference to the object it is
    // acessing, which the source object knows nothing about.
    Accessor(const Accessor&);
  };
}

#ifdef _CLASSDESC
#pragma omit pack ecolab::Accessor
#pragma omit unpack ecolab::Accessor
#pragma omit TCL_obj ecolab::Accessor
#endif

namespace classdesc_access
{
  namespace cd=classdesc;
  template <class T, class G, class S>
  struct access_pack<ecolab::Accessor<T,G,S> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T, class G, class S>
  struct access_unpack<ecolab::Accessor<T,G,S> >: 
    public cd::NullDescriptor<cd::unpack_t> {};
}
  
#endif
