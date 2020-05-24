/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* a series of TCL commands for accessing members of a C++
object. Accessed by the macro make_model(x), where x is the
name of a C++ object.

*/
/**\file
\brief TCL access descriptor
*/

#ifndef TCL_OBJ_BASE_H
#define TCL_OBJ_BASE_H
#include "tcl++.h"
#include "pack_base.h"
#include "pack_stl.h"
#include "ref.h"
#include "error.h"
#include "classdesc.h"

#include "isa_base.h"
#include "function.h"
#include <iostream>
#include <sstream>

/* define this macro to x to track TCL_obj registrations */
#define TCL_OBJ_DBG(x)

// ensure TCL_obj_templates is included, or linktime failure
namespace
{
  int TCL_obj_template_not_included();
  int dummyXXX=TCL_obj_template_not_included();
}

namespace ecolab
{
  using namespace classdesc;
  using classdesc::enable_if;
  using functional::bound_method;
  using functional::Return;
  /* classes for wrapping C++ functions, suitable for passing to
     Tcl_CreateCommand */

  template <class T>
  void ensure_exists(T*& x)
  {if (x==NULL) x=new T;}

  /* utility macro for declaring references to objects referred to in TCL
     arguments */

#define declare(name, typename, tcl_name)                               \
  typename *name##_entry;                                               \
  if (TCL_obj_properties().count(const_cast<char*>(tcl_name))==0)       \
    throw error("%s does not exist!",tcl_name);                         \
  name##_entry=TCL_obj_properties()[tcl_name]->memberPtrCasted<typename>(); \
  if (!name##_entry)                                                    \
    throw error("Incorrect argument %s assigned to %s",tcl_name,#name); \
  typename& name=*name##_entry;

  int TCL_proc(ClientData cd, Tcl_Interp *interp, int argc, CONST84 char **argv);
  int TCL_oproc(ClientData cd, Tcl_Interp *interp, int argc, 
                Tcl_Obj *const argv[]);  
  void TCL_delete(ClientData cd);

  inline void TCL_cmd_data_delete(ClientData cd)
  {
    delete static_cast<cmd_data*>(cd);
  }

  /* TCL_args - support for TCL object types:
     Use >> notation to extract arguments */

#ifndef TCL_MAJOR_VERSION
#error TCL not found.
#endif
#if (TCL_MAJOR_VERSION<8)
#error TCL 8.x or greater supported. Please upgrade your TCL
#endif

  /* used for testing whether simple or compound type */

#ifdef _CLASSDESC
#pragma omit pack ecolab::TCL_args
#pragma omit unpack ecolab::TCL_args
#pragma omit TCL_obj ecolab::TCL_args
#endif

  /// RAII TCL_Obj ref
  class TCLObjRef
  {
    Tcl_Obj* ref;
  public:
    TCLObjRef(): ref(Tcl_NewStringObj("",0)) {Tcl_IncrRefCount(ref);}
    TCLObjRef(Tcl_Obj* x): ref(x) {Tcl_IncrRefCount(x);}
    ~TCLObjRef() {Tcl_DecrRefCount(ref);}
    TCLObjRef(const TCLObjRef& x): ref(x.ref) {Tcl_IncrRefCount(ref);}
    TCLObjRef& operator=(const TCLObjRef& x) {
      ref=x.ref;
      Tcl_IncrRefCount(ref);
      return *this;
    }
    Tcl_Obj* get() const {return ref;}
  };

  /// count class that exposes const public attribute
  class TCL_args_count
  {
  protected:
    int m_count;
  public:
    TCL_args_count(int c=0): m_count(c), count(m_count) {}
    TCL_args_count(const TCL_args_count& x): m_count(x.count), count(m_count) {}
    void operator=(const TCL_args_count& x) {m_count=x.count;}
    const int& count;
  };
   
  
  /**
     \brief Represent arguments to TCL commands

     Declare a member function taking a single argument of type
     TCL_args, the extract the arguments using operator>>, or assign the
     arguments 1 by 1, or use operator[] to access

     \code
     struct foo
     {
     void bar1(TCL_args args)
     {
     int arg1=args, arg2=args;
     }
     int bar2(TCL_args args)
     {
     int arg1, arg2;
     args>>arg1>>arg2;
     }
     int bar3(TCL_args args)
     {
     int arg1=args[0], arg2=args[1];
     }
     }
     \endcode

     args[-1] refers to the command name.
  */
  class TCL_args: public TCL_args_count
  {
    int nextArg;
    std::vector<TCLObjRef> argv;
    Tcl_Obj *  pop_arg()
    {
      if (m_count>0) 
        {m_count--; return argv[nextArg++].get();} 
      else 
        throw error("too few arguments");
    }
    CLASSDESC_ACCESS(TCL_args);
    
  public:
    TCL_args(): nextArg(1), argv(1) {}
    TCL_args(int a, Tcl_Obj *const *v): TCL_args_count(a), nextArg(1)
    {
      m_count--;
      for (int i=0; i<a; ++i)
        argv.push_back(v[i]);
    }
    TCL_args operator[](int i) const
    {
      if (count<=i) 
        throw error("too few arguments");
      else
        {
          TCL_args r;
          r.pushObj(argv[i+nextArg].get());
          return r;
        }
    }

    void pushObj(Tcl_Obj* obj) {argv.push_back(obj); m_count++;}
    const char* str(); 

    TCL_args& operator<<(const std::string& x) 
    {pushObj(Tcl_NewStringObj(x.c_str(),-1)); return *this;}
    TCL_args& operator<<(const char* x) 
    {pushObj(Tcl_NewStringObj(x,-1)); return *this;}
    TCL_args& operator<<(bool x) {pushObj(Tcl_NewBooleanObj(x)); return *this;}
    TCL_args& operator<<(int x) {pushObj(Tcl_NewIntObj(x)); return *this;}
    TCL_args& operator<<(unsigned x) {pushObj(Tcl_NewIntObj(x)); return *this;}
    TCL_args& operator<<(long x) {pushObj(Tcl_NewLongObj(x)); return *this;}
    TCL_args& operator<<(double x) {pushObj(Tcl_NewDoubleObj(x)); return *this;}


    TCL_args& operator>>(std::string& x) {x=str(); return *this;}
    TCL_args& operator>>(const char*& x) {x=str(); return *this;}
    TCL_args& operator>>(bool& x) {
      int tmp;
      if (Tcl_GetBooleanFromObj(interp(),pop_arg(),&tmp)!=TCL_OK) 
        throw error("argument error");
      x=tmp;
      return *this;
    }
    TCL_args& operator>>(int& x) {
      if (Tcl_GetIntFromObj(interp(),pop_arg(),&x)!=TCL_OK) 
        throw error("argument error");
      return *this;
    }
    TCL_args& operator>>(unsigned& x) {
      int tmp;
      if (Tcl_GetIntFromObj(interp(),pop_arg(),&tmp)!=TCL_OK) 
        throw error("argument error");
      if (tmp>=0) x=tmp;
      else throw error("assigning %d to an unsigned variable",tmp);
      return *this;
    }
    TCL_args& operator>>(long& x) {
      if (Tcl_GetLongFromObj(interp(),pop_arg(),&x)!=TCL_OK) 
        throw error("argument error");
      return *this;
    }
    TCL_args& operator>>(double& x) {
      if (Tcl_GetDoubleFromObj(interp(),pop_arg(),&x)!=TCL_OK) 
        throw error("argument error");
      return *this;
    }
  //    TCL_args& operator>>(float& x) {x=*this; return *this;}
    template <class T>
    typename enable_if<is_rvalue<T>, T>::T
    get(dummy<0> d=0) {T x; *this>>x; return x;}

    template <class T>
    typename enable_if<Not<is_rvalue<T> >, T>::T
    get(dummy<1> d=0) 
    {throw error("calling get on %s", typeName<T>().c_str());}

    template <class T> operator T() {return get<T>();}
  };

  template <class T> TCL_args& operator>>(TCL_args& a, T& x);
  template <> inline TCL_args& operator>>(TCL_args& a, char*& x) {x=const_cast<char*>(a.str()); return a;}
  template <> inline TCL_args& operator>>(TCL_args& a, const char*& x) {x=a.str(); return a;}


  /// parallel declarator support for TCL_args 
  inline void parallel(TCL_args args)
  {
#ifdef MPI_SUPPORT
    if (myid==0)
      {
        eco_strstream s;
        s << (char*)args[-1];
        while (args.count) s << (char*)args;
        parsend(s.str());
      }
#endif
  }

  struct member_entry_base: public cmd_data
  {
    virtual void get() {throw error("get() not implemented");}
    virtual void put(const char *s) {throw error("put not implemented");}
    void proc(int argc, CONST84 char **argv)
    {
      if (argc>1) put(argv[1]);
      else get();
      if (hook) {hook(argc,argv);}
    }
    /// just to stop some compiler warning
    virtual void proc(int argc, Tcl_Obj *const argv[]) {
      cmd_data::proc(argc,argv);
      if (thook) thook(argc,argv);
    }
    /// a hook that is called whenever proc is called, to determine
    /// further processing
    void (*hook)(int argc, CONST84 char **argv);
    void (*thook)(int argc, Tcl_Obj *const argv[]);
    member_entry_base(): hook(NULL), thook(NULL) {is_setterGetter=true;}

    // std::type_info does not provide an overload for std::Less, so provide one here
    struct TypeInfoLess
    {
      bool operator()(const std::type_info* x, const std::type_info* y) const {
        return x->before(*y);
      }
    };

    /// map of pointers to base class objects of the referred object
    typedef std::map<const std::type_info*,void*,TypeInfoLess> BasePtrs;
    BasePtrs basePtrs;
    /// returns reference to base object of type T, if this is castable, null otherwise
    template <class T> T* memberPtrCasted() const;
  };
#ifdef _CLASSDESC
#pragma omit pack ecolab::member_entry_base
#pragma omit unpack ecolab::member_entry_base
#pragma omit pack TCL_obj_member_entry::member_entry
#pragma omit unpack TCL_obj_member_entry::member_entry
#pragma omit pack ecolab::member_entry
#pragma omit unpack ecolab::member_entry
#endif

#include <string>
  // fix shared_ptr type to prevent inconsistency in C++11 build environments
  // TODO convert to std::shared_ptr once EcoLab is C++11 only
  typedef std::map<string,classdesc::shared_ptr<member_entry_base> > TCL_obj_hash;

  TCL_obj_hash& TCL_obj_properties();

  // erase all object properties starting with \a name
  inline void eraseAllNamesStartingWith(const string& name)
  {
    for (TCL_obj_hash::iterator it=TCL_obj_properties().find(name);
         it!=TCL_obj_properties().end() && it->first.find(name)!=string::npos;)
      {
        TCL_obj_hash::iterator toErase=it++;
        TCL_obj_properties().erase(toErase);
      }
    
  }


  struct TCL_obj_checkr_base
  {
    virtual void pack(classdesc::pack_t&)=0;
    virtual void unpack(classdesc::pack_t&)=0;
    virtual ~TCL_obj_checkr_base() {}
  };

} // namespace ecolab

namespace classdesc
{
  /// TCL_obj descriptor object 
  class TCL_obj_t
  {
  public:
    ecolab::TCL_obj_checkr_base *check_functor;
    /// whether to use xdr_pack or binary pack in checkpoint/restart
    bool xdr_check;
    /// hook function to use whenever a setter/getter is called from TCL
    typedef void (*Member_entry_hook)(int argc, CONST84 char **argv);
    Exclude<Member_entry_hook> member_entry_hook;
    typedef void (*Member_entry_thook)(int argc, Tcl_Obj *const argv[]);
    Exclude<Member_entry_thook> member_entry_thook;

    /// checkpoint object to filename
    void checkpoint(int argc, char *argv[]);
    /// reload object from file
    void restart(int argc, char *argv[]);
    /// load object from a remote master copy (\a server, \a port)
    void get_vars(int argc, char *argv[]);
    /// attach to \a port so as to service \c get_vars requests from remote client
    void data_server(int argc, char *argv[]);
    TCL_obj_t(): xdr_check(false), member_entry_hook((Member_entry_hook)NULL),  
                 member_entry_thook((Member_entry_thook)NULL){}
  };
}

#ifdef _CLASSDESC
#pragma omit pack classdesc::TCL_obj_t
#pragma omit unpack classdesc::TCL_obj_t
#endif

namespace classdesc_access
{
  template <> struct access_pack<classdesc::TCL_obj_t>: 
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <> struct access_unpack<classdesc::TCL_obj_t>: 
    public classdesc::NullDescriptor<classdesc::unpack_t> {};
  

  template <class T> struct access_TCL_obj
  {
    /* by default, do nothing (unstructured types are just registered in TCL_obj()) */
    template <class U>
    void operator()(classdesc::TCL_obj_t& t,const classdesc::string& desc, U& arg) {}
  };

  // const type version
  template <class T> struct access_TCL_obj<const T>
  {
    /* by default, do nothing (unstructured types are just registered in TCL_obj()) */
    void operator()(classdesc::TCL_obj_t& t,const classdesc::string& desc, 
                    const T& arg) 
    {
      access_TCL_obj<T>()(t,desc,arg);
    }
  };
}

namespace ecolab
{

  /// a null TCL_obj_t suitable for nothing if needed.
  extern classdesc::TCL_obj_t null_TCL_obj;

  using classdesc::TCL_obj_t;
  using classdesc::pack;
  using classdesc::unpack;
  using classdesc::string;

  template <class T> void TCL_obj(TCL_obj_t&, const string&, T&);
  template <class T> void TCL_obj_onbase(TCL_obj_t&, const string&, T&);

#include "ref.h"
  ///  An EcoLab ref is a classdesc::ref, but also tracks change of pointee
  ///  to enable TCL commands to track the changes
  template <class T>
  struct ref: public classdesc::ref<T>
  {
    classdesc::string name;
    T& operator*() {
      if (this->nullref()) {
        TCL_obj(null_TCL_obj,name,classdesc::ref<T>::operator*());
      } 
      return classdesc::ref<T>::operator*();
    }
    const T& operator*() const {return classdesc::ref<T>::operator*();}
    T* operator->() {return &operator*();}
    const T* operator->() const {return &operator*();}
    ref() {}  
    ref(const ref& x): classdesc::ref<T>(static_cast<const classdesc::ref<T>&>(x)) {}
    template <class U> ref(const U& x): classdesc::ref<T>(x) {}
    ref& operator=(const ref& x)  {
      classdesc::ref<T>::operator=(static_cast<const classdesc::ref<T>&>(x)); 
      eraseAllNamesStartingWith(name);
      TCL_obj(null_TCL_obj,name,**this); 
      return *this;
    }
    template <class U> ref& operator=(const U& x) {
      classdesc::ref<T>::operator=(x); 
      eraseAllNamesStartingWith(name);
      TCL_obj(null_TCL_obj,name,**this); 
      return *this;
    }
    void swap(ref& x) {
      classdesc::ref<T>::swap(x);
      eraseAllNamesStartingWith(name);
      TCL_obj(null_TCL_obj,name,**this);
      eraseAllNamesStartingWith(x->name);
      TCL_obj(null_TCL_obj,x->name,*x);
    }
    template <class U>
    bool operator==(const U& x) {return classdesc::ref<T>::operator==(x);}
    template <class U>
    bool operator!=(const U& x) {return !operator==(x);}
  };

  /** 
      \brief a slightly safer way of referring to registered objects
      than bare pointers

      use this as a receptacle for TCLTYPED objects created in the TCL
      interface, and then use as a smart pointer in the C++ code
  */
  template <class T>
  class TCL_obj_ref
  {
    T* datum;
    std::string name;
    TCL_obj_ref(T& x, const char* nm): datum(&x), name(nm) {}
  public:
    TCL_obj_ref(): datum(NULL) {}
    TCL_obj_ref(const char* nm): datum(NULL) {set(nm);}
    T* operator->() {return datum;}
    T& operator*() {return *datum;}
    const T* operator->() const {assert(datum); return datum;}
    const T& operator*() const {assert(datum); return *datum;}
    operator bool() const {return datum;} 
    /// set object to refer to registered object \a s.
    void set(const char *s);
    void pack(classdesc::pack_t& buf);
    void unpack(classdesc::pack_t& buf);
  };

#ifdef _CLASSDESC
#pragma omit pack ecolab::TCL_obj_ref
#pragma omit unpack ecolab::TCL_obj_ref
#pragma omit TCL_obj ecolab::TCL_obj_ref
#pragma omit TCL_obj classdesc::ref
#pragma omit TCL_obj ecolab::ref
#endif

  template <class T> struct ref;

  /* member_entry is defined in its own namespace, as 
     template<class T> void operator<<(ostream& x,const T& y) 
     conflicts with hash_map */
  

    //  template<class T> inline eco_strstream& operator|(eco_strstream& x,const T& y);
  template<class T, class CharT, class Traits> 
  typename enable_if<
    And<Not<is_enum<T> >, Not<is_container<T> > >,
    std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y); 

  template<class T, class CharT, class Traits> 
  typename enable_if<is_sequence<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y); 

  template<class T, class CharT, class Traits> 
  typename enable_if<is_associative_container<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y); 

  template<class T, class CharT, class Traits> 
  typename enable_if<is_enum<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y); 

  template<class T, class CharT, class Traits> 
  typename enable_if<is_enum<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y); 


  template<class T, class CharT, class Traits> 
  inline std::basic_istream<CharT,Traits>& 
  operator>>(std::basic_istream<CharT,Traits>&, ref<T>& y);

  template <class T> struct member_entry: public member_entry_base
  {
    T *memberptr;
    member_entry(): memberptr(0) {}
    member_entry(T& x) {memberptr=&x;}
    void get();
    void put(const char *s); 
  };
  
  template <class T> T* member_entry_base::memberPtrCasted() const
  {
    if (const member_entry<T>* m=dynamic_cast<const member_entry<T>*>(this)) 
      return m->memberptr;
    else
      {
        BasePtrs::const_iterator i=basePtrs.find(&typeid(T));
        if (i!=basePtrs.end())
          return (T*)(i->second);
        else
          return NULL;
      }
  }


  template <class T> struct member_entry<const classdesc::Enum_handle<T> >: 
    public member_entry_base
  {
    Enum_handle<T> *memberptr;
    member_entry(): memberptr(0) {}
    member_entry(const Enum_handle<T>& x) {memberptr=new Enum_handle<T>(x);}
    ~member_entry() {delete memberptr;}
    void get();
    void put(const char *s); 
  };

  template <class T> struct member_entry<const classdesc::Enum_handle<const T> >: 
    public member_entry_base
  {
    Enum_handle<const T> *memberptr;
    member_entry(): memberptr(0) {}
    member_entry(const Enum_handle<const T>& x) {memberptr=new Enum_handle<const T>(x);}
    ~member_entry() {delete memberptr;}
    void get() {tclreturn()<<string(*memberptr);}
    void put(const char *s) {throw error("cannot change const attribute");}
  };


  template <class T> struct member_entry<const T>: public member_entry_base
    {
      const T data; // holds copy of data, in case a temporary is passed
      const T *memberptr;
      member_entry(): memberptr(0) {}
      member_entry(const T& x): data(x), memberptr(&data) {}
      void get();
      void put(const char *s) {throw error("cannot change const attribute");}
    };
  
    template <class T>
    struct member_entry<ecolab::TCL_obj_ref<T> >: public member_entry_base
    {
      TCL_obj_ref<T> *memberptr;
      member_entry(): memberptr(0) {}  
      member_entry(TCL_obj_ref<T>& x) {memberptr=&x;}
      /* You can assign registered TCL_objs to TCL_obj_ref<T> */
      void put(const char *s) {memberptr->set(s); tclreturn() << s;}
    };
  
    template <class T>
    struct member_entry<T*>: public member_entry_base
    {
      T **memberptr;
      member_entry(): memberptr(0) {}  
      member_entry(T*& x) {memberptr=&x;}
      /* You can assign registered TCL_objs to pointers */
      void put(const char *s)
      {
        if (!memberptr)
          throw error("missing reference to assign %s to",s);
        //	member_entry<T> *object_entry;
	/*ensure_exists(TCL_obj_properties());*/
	if (TCL_obj_properties().count(s)==0)
	  throw error("%s does not exist!",s);
	if (T *object=TCL_obj_properties()[s]->memberPtrCasted<T>())
          {
            *memberptr=object;
            tclreturn() << s;
          }
        else
	  throw error("Incorrect argument type %s",s); 
      }
    };
  
    template <>
    struct member_entry<void*>: public member_entry_base
    {
      void *memberptr;
      member_entry(): memberptr(0) {}
      member_entry(void*& x) {memberptr=x;}
    };
  
        template <>
    struct member_entry<const void*>: public member_entry_base
    {
      const void *memberptr;
      member_entry(): memberptr(0) {}
      member_entry(const void*& x) {memberptr=x;}
    };

    template <>
    struct member_entry<void>: public member_entry_base
    {
      void *memberptr;
      member_entry(): memberptr(0) {}  
      member_entry(void*& x) {memberptr=x;}
    };


  template <class T>
  void TCL_obj_ref<T>::set(const char *s) {
    if (TCL_obj_properties().count(s)==0)
      throw error("%s does not exist!",s);
    if (T* object= TCL_obj_properties()[s]->memberPtrCasted<T>())
      {
        datum=object;
        name=s;
      }
    else
      throw error("Incorrect argument type %s",s); 
  }

  template <class T>
  void TCL_obj_register(const TCL_obj_t& targ, const string& desc, T& arg)
  {
    member_entry<T> *m=new member_entry<T>(arg);
    m->hook=targ.member_entry_hook;
    m->thook=targ.member_entry_thook;
    m->name=desc;
    TCL_OBJ_DBG(printf("registering %s, with entry %x\n",desc.c_str(),m));
    //  assert(TCL_newcommand(desc)); /* we just want the latest resgistration */
    Tcl_CreateCommand(interp(),desc.c_str(),(Tcl_CmdProc*)TCL_proc,(ClientData)m,TCL_delete);
    TCL_obj_properties()[desc].reset(m);
  }

  template <class T>
  void TCL_obj_registerBase(const TCL_obj_t& targ, const string& desc, T& arg)
  {
    TCL_obj_hash::iterator it=TCL_obj_properties().find(desc);
    if (it==TCL_obj_properties().end())
      TCL_obj_register(targ,desc,arg);
    else // registering a base class
      {
        assert(it->second);
        // this is presumably a base class registration
        it->second->basePtrs[&typeid(T)]=(void*)&arg;
      }
  }

  
  
  void TCL_obj_deregister(const string& desc );

  /// a 'hook' to allow registrations to occur for TCL_objects (overriding base classes)
  template <class T> void TCL_obj_custom_register(const string& desc, T& arg) {}


  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_array ia, 
               T &arg,int dims,...);

  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_const_static s, const T& t)
  {
    TCL_obj(targ, desc, t);
  }

  /// const static method support
  template <class T, class U>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_const_static s, const T& t, U u)
  {
    TCL_obj(targ, desc, u);
  }

  /* deal with treenode pointers */
  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_treenode dum, T& arg);

  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_graphnode dum, T& arg);

#ifdef _CLASSDESC
#pragma omit pack ecolab::TCL_obj_functor
#pragma omit unpack ecolab::TCL_obj_functor
#pragma omit TCL_obj ecolab::TCL_obj_functor
#pragma omit isa ecolab::TCL_obj_functor
#endif
  template<class C, class T>
  struct TCL_obj_functor: public cmd_data
  {
    C *o;
    functor_class c;
    union {
      T (C::*mbrvoid) ();
      T (C::*mbr)(int,char**);
      T (C::*mbrobj)(TCL_args);  
      T (*fptr)(...);
      T (*ofptr)(const TCL_args&);  
    };
    void (*hook)(int argc, CONST84 char **argv);
    void (*thook)(int argc, Tcl_Obj *const argv[]);

    TCL_obj_functor(): hook(NULL), thook(NULL) {c=invalid;}
    void init(C& oo, T (C::*m) ()) {o=&oo; mbrvoid=m; c=memvoid;}
    void init(C& oo, T (C::*m) (int,char**)) {o=&oo; mbr=m; c=mem;}
    void init(C& oo, T (C::*m) (TCL_args)) {o=&oo; mbrobj=m; c=mem;}
    void init(C& oo, T (*f) (...)) {o=&oo; fptr=f; c=func;}
    void init(C& oo, T (*f) (const TCL_args&)) {o=&oo; ofptr=f; c=func;}
    void proc(int argc, CONST84 char **argv)
    {
      tclreturn r;
      switch (c)
        {
        case memvoid: r<<(o->*mbrvoid)(); break;
        case mem: r<<(o->*mbr)(argc,const_cast<char**>(argv)); break;
        case func: r<<fptr(argc,const_cast<char**>(argv)); break;
        case nonconst: throw error("non const method called on const object");
        default: break;
        }
      if (hook) hook(argc, argv);
    }
    void proc(int argc, Tcl_Obj *const argv[])
    {
      tclreturn r;
      switch (c)
        {
        case mem: r<<(o->*mbrobj)(TCL_args(argc,argv)); break;
        case func: r<<ofptr(TCL_args(argc,argv)); break;
        case nonconst: throw error("non const method called on const object");
        default: break;
        }
      if (thook) thook(argc, argv);
    }
  };

  template<class C, class T>
  struct TCL_obj_functor<const C,T>: public cmd_data
  {
    const C *o;
    functor_class c;
    union {
      T (C::*mbrvoid) () const;
      T (C::*mbr)(int,char**) const;
      T (C::*mbrobj)(TCL_args) const;  
      T (*fptr)(...);
      T (*ofptr)(const TCL_args&);  
    };
    void (*hook)(int argc, CONST84 char **argv);
    void (*thook)(int argc, Tcl_Obj *const argv[]);

    TCL_obj_functor(): hook(NULL), thook(NULL) {c=invalid;}
    void init(const C& oo, T (C::*m) () const) {o=&oo; mbrvoid=m; c=memvoid;}
    void init(const C& oo, T (C::*m) (int,char**) const) {o=&oo; mbr=m; c=mem;}
    void init(const C& oo, T (C::*m) (TCL_args) const ) {o=&oo; mbrobj=m; c=mem;}
    void init(const C& oo, T (C::*m) ()) {o=&oo; c=nonconst;}
    void init(const C& oo, T (C::*m) (int,char**)) {o=&oo; c=nonconst;}
    void init(const C& oo, T (C::*m) (TCL_args)) {o=&oo; c=nonconst;}
    void init(const C& oo, T (*f) (...)) {o=&oo; fptr=f; c=func;}
    void init(const C& oo, T (*f) (const TCL_args&)) {o=&oo; ofptr=f; c=func;}
    void proc(int argc, CONST84 char **argv)
    {
      tclreturn r;
      switch (c)
        {
        case memvoid: r<<(o->*mbrvoid)(); break;
        case mem: r<<(o->*mbr)(argc,const_cast<char**>(argv)); break;
        case func: r<<fptr(argc,const_cast<char**>(argv)); break;
        }
      if (hook) hook(argc, argv);
    }
    void proc(int argc, Tcl_Obj *const argv[])
    {
      tclreturn r;
      switch (c)
        {
        case mem: r<<(o->*mbrobj)(TCL_args(argc,argv)); break;
        case func: r<<ofptr(TCL_args(argc,argv)); break;
        }
      if (thook) thook(argc, argv);
    }
  };



  template<class C>
  struct TCL_obj_functor<C,void>: public  cmd_data
  {
    C *o;
    functor_class c;
    union {
      void (C::*mbrvoid) ();
      void (C::*mbr)(int,char**);
      void (C::*mbrobj)(TCL_args);  
      void (*fptr)(...);
      void (*ofptr)(const TCL_args&);
    };
    void (*hook)(int argc, CONST84 char **argv);
    void (*thook)(int argc, Tcl_Obj *const argv[]);

    TCL_obj_functor(): hook(NULL), thook(NULL) {c=invalid;}
    void init(C& oo, void (C::*m) ()) {o=&oo; mbrvoid=m; c=memvoid;}
    void init(C& oo, void (C::*m) (int,char**)) {o=&oo; mbr=m; c=mem;}
    void init(C& oo, void (C::*m) (TCL_args)) {o=&oo; mbrobj=m; c=mem;}
    void init(C& oo, void (*f) (...)) {o=&oo; fptr=f; c=func;}
    void init(C& oo, void (*f) (const TCL_args&)) {o=&oo; ofptr=f; c=func;}
    void proc(int argc, CONST84 char **argv)
    {
      switch (c)
        {
        case memvoid: (o->*mbrvoid)(); break;
        case mem: (o->*mbr)(argc,const_cast<char**>(argv)); break;
        case func: fptr(argc,const_cast<char**>(argv)); break;
        default: break;
        }
      if (hook) hook(argc, argv);
    }
    void proc(int argc, Tcl_Obj *const argv[])
    {
      switch (c)
        {
        case mem: (o->*mbrobj)(TCL_args(argc,argv)); break;
        case func: ofptr(TCL_args(argc,argv)); break;
        default: break;
        }
      if (thook) thook(argc, argv);
    }
  };

  /// whether B's method is callable due to the rules of const-correctness, or due to having lvalue arguments
  template <class B> struct BoundMethodCallable: public false_type {};

  template <class C, class M>
  struct BoundMethodCallable<functional::bound_method<C,M> >: public
  And<Or<Not<is_const<C> >, functional::is_const_method<M> >,
  functional::AllArgs<functional::bound_method<C,M>, is_rvalue> > {};

#ifdef _CLASSDESC
#pragma omit pack ecolab::BoundMethodCallable
#pragma omit unpack ecolab::BoundMethodCallable
#pragma omit TCL_obj ecolab::BoundMethodCallable
#pragma omit isa ecolab::BoundMethodCallable
#endif

  // for methods returning a value
  template <class B, class A>
  typename enable_if
  <And<Not<is_void<typename Return<B>::T> >, BoundMethodCallable<B> >, void>::T
  newTCL_obj_functor_proc(B bm, A args, dummy<0> x=0)
  {
    tclreturn() << functional::apply_nonvoid_fn(bm, args);
  }

  // for ones that don't
  template <class B, class A>
  typename enable_if
  <And<is_void<typename Return<B>::T>, BoundMethodCallable<B> >,void>::T
  newTCL_obj_functor_proc(B bm, A args, dummy<1> x=0)
  {
     functional::apply_void_fn(bm, args);
  }

  // and ones that have lvalue arguments, or are not const-correctly callable
  template <class B, class A>
  typename enable_if<Not<BoundMethodCallable<B> >, void>::T
  newTCL_obj_functor_proc(B bm, A args, dummy<2> x=0)
  {
    throw error("cannot call %s",args[-1].str());
  }

  /* what to do about member functions */
  template <class C, class M>
  class NewTCL_obj_functor: public cmd_data
  {
    bound_method<C,M> bm;
    TCL_obj_t::Member_entry_thook thook;
  public:
    NewTCL_obj_functor(const TCL_obj_t& targ,C& obj, M member): 
      bm(obj, member), thook(targ.member_entry_thook) {
      if (functional::is_const_method<M>::value)
        is_const=true;
    }
    void proc(int argc, Tcl_Obj *const argv[]) {
      newTCL_obj_functor_proc(bm, TCL_args(argc, argv));
      if (thook) thook(argc, argv);
    }
    void proc(int, const char **) {}  
  };

  template<class C, class M>
  typename enable_if<is_member_function_pointer<M>, void>::T
  TCL_obj(TCL_obj_t& targ, const string& desc, C& c, M m) 
  {
    NewTCL_obj_functor<C,M> *t=new NewTCL_obj_functor<C,M>(targ,c,m);
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()));
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  } 
  
  // static methods
  template <class F>
  class NewTCL_static_functor: public cmd_data
  {
    F f;
    TCL_obj_t::Member_entry_thook thook;
  public:
    NewTCL_static_functor(const TCL_obj_t& targ,F f): 
      f(f), thook(targ.member_entry_thook) {is_const=true;}
    void proc(int argc, Tcl_Obj *const argv[]) {
      newTCL_obj_functor_proc(f, TCL_args(argc, argv));
      if (thook) thook(argc, argv);
    }
    void proc(int, const char **) {}  
  };

  template<class C, class M>
  typename enable_if<functional::is_nonmember_function_ptr<M>, void>::T
  TCL_obj(TCL_obj_t& targ, const string& desc, C& c, M m) 
  {
    NewTCL_static_functor<M> *t=new NewTCL_static_functor<M>(targ,m);
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()));
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  } 

  template<class M>
  typename enable_if<functional::is_nonmember_function_ptr<M>, void>::T
  TCL_obj(TCL_obj_t& targ, const string& desc, M m) 
  {
    NewTCL_static_functor<M> *t=new NewTCL_static_functor<M>(targ,m);
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()));
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  } 

  /** methods with signature (int,char**) or TCL_args can be used to
     handle methods with variable arguments, or ones with reference
     arguments */


  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj,
               T (C::*arg)(int,const char**)) 
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateCommand(interp(),desc.c_str(),(Tcl_CmdProc*)TCL_proc,(ClientData)t,TCL_cmd_data_delete);
  } 
  
  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj,
               T (C::*arg)(int,const char**) const) 
  {
    typedef T (C::*mptr)(int,const char**);
    TCL_obj(targ,desc,obj,(mptr)arg);
    TCL_obj_properties()[desc]->is_const=true;
  }

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj,
               T (C::*arg)(int,char**)) 
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateCommand(interp(),desc.c_str(),(Tcl_CmdProc*)TCL_proc,(ClientData)t,TCL_cmd_data_delete);
  } 

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj,
               T (C::*arg)(int,char**) const) 
  {
    typedef T (C::*mptr)(int,char**);
    TCL_obj(targ,desc,obj,(mptr)arg);
    TCL_obj_properties()[desc]->is_const=true;
  }

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj, T (*arg)(int argc, const char**))
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  }

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj, T (*arg)(int argc, char**))
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  }

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj,
               T (C::*arg)(TCL_args)) 
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  }
 
  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj,
               T (C::*arg)(TCL_args) const)
  { 
    typedef T (C::*mptr)(TCL_args);
    TCL_obj(targ,desc,obj,(mptr)arg);
    TCL_obj_properties()[desc]->is_const=true;
  }

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj, T (*arg)(TCL_args))
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  }

  template<class C, class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, C& obj, T (*arg)(const TCL_args&))
  {
    TCL_obj_functor<C,T> *t=new TCL_obj_functor<C,T>;
    t->init(obj,arg); t->name=desc;
    t->hook=targ.member_entry_hook; t->thook=targ.member_entry_thook;
    TCL_OBJ_DBG(printf("registering %s\n",desc.c_str()););
    Tcl_CreateObjCommand(interp(),desc.c_str(),TCL_oproc,(ClientData)t,TCL_cmd_data_delete);
  }

#ifdef _CLASSDESC
#pragma omit TCL_obj string
#pragma omit TCL_obj eco_strstream

  /* these classes cause problems because they don't have copy constructors */
#pragma omit TCL_obj ostream
#pragma omit TCL_obj ecolab::member_entry_base
#pragma omit TCL_obj _ios_fields

#pragma omit TCL_obj GRAPHCODE_NS::omap
#pragma omit TCL_obj graphcode::GraphMaps
#pragma omit TCL_obj graphcode::Graph
#pragma omit TCL_obj ref
#endif

  template <class T>
  void TCL_obj(TCL_obj_t& t, const string& d, const Enum_handle<T>& a)
  {TCL_obj_register(t,d, a);}
}

using ecolab::TCL_obj;
using ecolab::TCL_obj_onbase;

namespace classdesc_access
{
  namespace cd=classdesc;

  template <class T>
  struct access_TCL_obj<ecolab::ref<T> >
  {
    template <class U>
    void operator()(cd::TCL_obj_t& t, const cd::string& d, U& a)
    {
      a.name=d;
      // an EcoLab ref always has a target once it has been TCL_obj'd
      TCL_obj(t,d,*a);
    }
  };

  template <class T>
  struct access_TCL_obj<classdesc::shared_ptr<T> >
  {
    template <class U>
    void operator()(cd::TCL_obj_t& t, const cd::string& d, U& a)
    {
      if (a) TCL_obj(t,d,*a);
    }
  };

  template <class T>
  struct access_TCL_obj<classdesc::Enum_handle<T> >
  {
    template <class U>
    void operator()(cd::TCL_obj_t& t, const cd::string& d, U& a)
    {TCL_obj_register(d, a, t.member_entry_hook, t.member_entry_thook);}
  };
}


namespace ecolab
{
  /// distinguish between maps and sets based on value_type of container
  template <class T> struct is_map;

  /// redirect member pointers to old fashioned descriptor
  template<class C, class T>                                          
  typename enable_if<And<is_member_object_pointer<T>,                 
                         Not<functional::is_nonmember_function_ptr<T> > >,void>::T 
  TCL_obj(TCL_obj_t& b, const string& d, C& o, T y)           
  {TCL_obj(b,d,o.*y);}                                             

}

#include "TCL_obj_stl.h"

#if defined(__GNUC__) && !defined(__ICC) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#include "TCL_obj_base.cd"

#if defined(__GNUC__) && !defined(__ICC) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* TCL_OBJ_BASE_H */
