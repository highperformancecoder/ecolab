/*
  @copyright Russell Standish 2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ACCESSOR_H
#define ACCESSOR_H
#include "TCL_obj_base.h"
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

     Deprecated in favor of TCLAccessor
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

  /// for accessors (overloaded getter/setters that pretend to be attributes)
  template <class F>
  struct TCL_accessor: public cmd_data
  {
    const F& f;
    TCL_accessor(const F& f): f(f) {}
    void proc(int argc, Tcl_Obj *const argv[]) {
      tclreturn r;
      if (argc<=1)
        r << f();
      else
        r << f(TCL_args(argc, argv));
    }
    void proc(int, const char **) {}  
  };

  /// const version - only getter is called
  template <class F>
  struct TCL_accessor<const F>: public cmd_data
  {
    const F f;
    TCL_accessor(F f): f(f) {}
    void proc(int argc, Tcl_Obj *const argv[]) {
      tclreturn() << f();
    }
    void proc(int, const char **) {}  
  };

  /**
     use this like 
     class Item: public TCLAccessor<Item,double>
     {
     ...
     Item(): TCLAcessor<Item,double>("rotation",(Getter)&Item::rotation,(Setter)&Item::rotation) {}
     double rotation() const;
     double rotation(double);
     };

     @param T subclass of this we're inserting the accessor into
     @param V type of the accessor
     @param N used to distinguish base classes when multiple accessors are deployed:

     class Item: public TCLAccessor<Item,double,0>, public TCLAccessor<Item,double,1>
     {
     ...
     Item(): 
     TCLAcessor<Item,double,0>("rotation",
     (TCLAcessor<Item,double,0>::Getter)&Item::rotation,
     (TCLAcessor<Item,double,0>::Setter)&Item::rotation), 
     TCLAcessor<Item,double,1>("scale",
     (TCLAcessor<Item,double,1>::Getter)&scale::scale,
     (TCLAcessor<Item,double,1>Setter)&Item::scale) 
     {}
     ...
     };
  */

  template <class T, class V, int N=0>
  class TCLAccessor: public cmd_data
  {
  public:
    using Getter=V (T::*)() const;
    using Setter=V (T::*)(const V&);
    TCLAccessor(const std::string& name, Getter g, Setter s):
      name(name), _self(static_cast<T&>(*this)), g(g), s(s) {}
    TCLAccessor(const TCLAccessor& x):
      name(x.name), _self(static_cast<T&>(*this)), g(x.g), s(x.s) {}
    // assignment need do nothing, everything is set up for this
    TCLAccessor& operator=(const TCLAccessor& x) {return *this;}
    void proc(int argc, Tcl_Obj *const argv[]) {
      tclreturn r;
      if (argc<=1)
        r << (_self.*g)();
      else
        r << (_self.*s)(TCL_args(argc, argv));
    }
    void proc(int argc, Tcl_Obj *const argv[]) const {
      tclreturn()<<(_self.*g)();
    }
    void proc(int, const char **) {}  
    std::string name; ///< name of accessor seen by TCL
  private:
    T& _self; ///< reference to the outer object accessor are working on
    Getter g;   ///< getter - self.*g() should return a V
    Setter s;   ///< setter - self.*s(v) should set the attribute to v and return a V
  };

  
}

/** @{ convenience macros for handling multiple accessor declarations
    ECOLAB_ACESSOR_DECL(class,name,type) declares a type \a class_type that can be inherited in the class, which can then be initialised using ECOLAB_ACESSOR_INIT
\verbatim
    class Foo;
    ECOLAB_ACESSOR_DECL(Foo,bar,double);
    class Foo: public Foo_bar
    {
    public:
      Foo(): ECOLAB_ACESSOR_INIT(Foo,bar) {}
    }
\endverbatim 
 **/
#define ECOLAB_ACESSOR_DECL(class,name,type)                               \
  struct class##_##name: public ecolab::TCLAccessor<class,type>     \
  {                                                                 \
    class##_##name(const std::string& name, Getter g, Setter s):    \
      ecolab::TCLAccessor<class,type>(name,g,s) {}                  \
  };

#define ECOLAB_ACESSOR_INIT(class,name)                                    \
  class##_##name(#name, (class##_##name::Getter)&class::name, (class##_##name::Setter)&class::name)

/// @}

#ifdef _CLASSDESC
#pragma omit pack ecolab::Accessor
#pragma omit unpack ecolab::Accessor
#pragma omit TCL_obj ecolab::Accessor
#pragma omit pack ecolab::TCLAccessor
#pragma omit unpack ecolab::TCLAccessor
#pragma omit TCL_obj ecolab::TCLAccessor
#endif

namespace classdesc
{
  template <class T, class G, class S>
  struct tn<ecolab::Accessor<T,G,S> >
  {
    static string name() {return "ecolab::Accessor<"+typeName<T>()+","+
        typeName<G>()+","+typeName<S>()+">";}
  };
  template <class T, class V, int N>
  struct tn<ecolab::TCLAccessor<T,V,N> >
  {
    static string name() {return "ecolab::TCLAccessor<"+typeName<T>()+","+
        typeName<V>()+">";}
  };
}

namespace classdesc_access
{
  namespace cd=classdesc;
  template <class T, class G, class S>
  struct access_pack<ecolab::Accessor<T,G,S> >: 
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T, class G, class S>
  struct access_unpack<ecolab::Accessor<T,G,S> >: 
    public cd::NullDescriptor<cd::unpack_t> {};

  template <class T, class G, class S>
  struct access_TCL_obj<ecolab::Accessor<T,G,S> >
   {
     template <class U>
     void operator()(cd::TCL_obj_t& t, const cd::string& d, U& a)
     {
       TCL_OBJ_DBG(printf("registering %s\n",d.c_str()););
       Tcl_CreateObjCommand(ecolab::interp(),d.c_str(),ecolab::TCL_oproc,
                            (ClientData)new ecolab::TCL_accessor<U>(a),
                            ecolab::TCL_cmd_data_delete);
     }
  };

  
  template <class T, class V, int N>
  struct access_pack<ecolab::TCLAccessor<T,V,N> >:
    public cd::NullDescriptor<cd::pack_t> {};
  template <class T, class V, int N>
  struct access_unpack<ecolab::TCLAccessor<T,V,N> >:
    public cd::NullDescriptor<cd::unpack_t> {};

  template <class T, class V, int N>
  struct access_TCL_obj<ecolab::TCLAccessor<T,V,N> >
  {
    template <class U>
    void operator()(classdesc::TCL_obj_t&, const std::string& d, U& a)
    {
      Tcl_CreateObjCommand(ecolab::interp(),(d+"."+a.name).c_str(),ecolab::TCL_oproc,(ClientData)&a,nullptr);
     }
  };
}
  
#endif
