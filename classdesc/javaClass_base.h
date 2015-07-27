/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
  \brief java class descriptor

  generates a java class file describing the interface of a C++ object
*/
#ifndef JAVACLASS_BASE_H
#define JAVACLASS_BASE_H

#include "javaClass.h"
#include "javaClassDescriptor.h"
#include "object.h"
#include "poly.h"
#include <memory>
#include <jni.h>

// for debugging use
//namespace std
//{
//  template <class T, class C, class traits> 
//  basic_ostream<C,traits>& operator<<(basic_ostream<C,traits>& o, const T& x) 
//  {o<<"<unknown type>"; return o;}
//}

namespace classdesc
{
  template <class T> const char* javaClassName();
  template <> inline const char* javaClassName<int>() {return "java/lang/Integer";}
  template <> inline const char* javaClassName<float>() {return "java/lang/Float";}

  template <class T> T getJavaVal(JNIEnv *env, jobject o);
  template <> inline int getJavaVal<int>(JNIEnv *env, jobject o) {
      return env->CallIntMethod(o, 
                env->GetMethodID(env->GetObjectClass(o), "intValue", "()I"));
  }
  template <> inline float getJavaVal<float>(JNIEnv *env, jobject o) {
    return env->CallFloatMethod(o, 
            env->GetMethodID(env->GetObjectClass(o), "floatValue", "()F"));
  }
  template <> inline std::string getJavaVal<std::string>(JNIEnv *env,jobject o)
  {
    return std::string(env->GetStringUTFChars(static_cast<jstring>(o),NULL));
  }

  template <class T> T getJavaVal(jvalue v);
  template <> inline bool getJavaVal<bool>(jvalue v) {return v.z;}
  template <> inline char getJavaVal<char>(jvalue v) {return v.b;}
  template <> inline unsigned short getJavaVal<unsigned short>(jvalue v) {return v.c;}
  template <> inline short getJavaVal<short>(jvalue v) {return v.s;}
  template <> inline int getJavaVal<int>(jvalue v) {return v.i;}
  template <> inline long getJavaVal<long>(jvalue v) {return v.j;}
  template <> inline float getJavaVal<float>(jvalue v) {return v.f;}
  template <> inline double getJavaVal<double>(jvalue v) {return v.d;}

  /// base class for \c ArgRef
  struct ArgRef_base: virtual public object //Object<ArgRef_base,0>
  {
    virtual jobject get_jobject(JNIEnv *env) const=0;
    virtual jint get_jint(JNIEnv *env) const {return 0;} 
    virtual jfloat get_jfloat(JNIEnv *env) const {return 0;} 
    virtual ~ArgRef_base() {};
  };

  /**
     used for handling return values.  
     TODO: type identifier system subverted, should this be Auto_type_object 
     instead?
  */
  template <class T>
  class ArgRef: virtual public Object<ArgRef<T>,ArgRef_base> 
  {
    T value;
    friend class RetRef;
  public:
    ArgRef() {}
    ArgRef(const T& x): value(x) {}
    jobject get_jobject(JNIEnv *env) const {
      jclass clss=env->FindClass(javaClassName<T>());
      return env->NewObject(clss,
                          env->GetMethodID(
                                       clss,"<init>", 
                             (std::string("(")+descriptor<T>()+")V").c_str()), 
                             value);
    }
    jint get_jint(JNIEnv *env) const {return jint(value);}
    jfloat get_jfloat(JNIEnv *env) const {return jfloat(value);}
      
    T* getRef() {return &value;}
  };

  template <>
  class ArgRef<void>: virtual public Object<ArgRef<void>,ArgRef_base>
  {
  public:
    jobject get_jobject(JNIEnv *env) const {return NULL;}
    void* getRef() {return NULL;}
  };
}
#ifdef _CLASSDESC
#pragma omit pack classdesc::ArgRef
#pragma omit unpack classdesc::ArgRef
#pragma omit javaClass classdesc::ArgRef
#pragma omit javaClass classdesc::ArgRef_base
#endif

namespace classdesc_access
{
  //no sensible serialisation for this
  template <class T>
  struct access_pack<classdesc::ArgRef<T> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class T>
  struct access_unpack<classdesc::ArgRef<T> >:
    public classdesc::NullDescriptor<classdesc::unpack_t> {};
}

namespace classdesc
{
  //utility class for casting jobjects into C++ types
  class Arg
  {
    JNIEnv *env;
    jvalue val;
    bool is_jobject;
  public:

    Arg(): env(NULL), is_jobject(true) {val.l=NULL;}
    Arg(JNIEnv *env, jobject object): env(env), is_jobject(true) {val.l=object;}
    //    Arg(JNIEnv *env, jboolean v): env(env), is_jobject(false) {val.z=v;}
    Arg(JNIEnv *env, jbyte v): env(env), is_jobject(false) {val.b=v;}
    Arg(JNIEnv *env, jchar v): env(env), is_jobject(false) {val.c=v;}
    Arg(JNIEnv *env, jint v): env(env), is_jobject(false) {val.i=v;}
    Arg(JNIEnv *env, jlong v): env(env), is_jobject(false) {val.j=v;}
    Arg(JNIEnv *env, jfloat v): env(env), is_jobject(false) {val.f=v;}
    Arg(JNIEnv *env, jdouble v): env(env), is_jobject(false) {val.d=v;}

    template <class T> T get() const {
      if (is_jobject)
        return getJavaVal<T>(env,val.l);
      else
        return getJavaVal<T>(val);
    }

    //TODO: factor out get<T>()
    operator float() {return get<float>();}
    operator int() {return get<int>();}
    operator std::string() {return get<std::string>();}

    //TODO: handle reference arguments
  };

  /// Handle the return value
  class RetVal: public poly<ArgRef_base>
  {
  public:
    template <class T> T* getRef() {
      addObject<ArgRef<T> >();
      return cast<ArgRef<T> >().getRef();
    }
  };

  /// Convert a C++ type to a Java JNI type.
  template <class Jtype>
  struct JNItype
  {
    template <class CppType>
    static Jtype from(JNIEnv *env, const CppType& x) {return JNItype(x);}
  };

  /// specialisation for objects. TODO - consider attaching an interface
  ///  TODO lifetime issues?
  template <>
  struct JNItype<jobject>
  {
    template <class CppType>
    static jobject from(JNIEnv *env, const CppType& x)
    {
      jclass cls=env->FindClass("ConcreteCppObject");
      jmethodID constructor=env->GetMethodID(cls,"<init>","()V");
      jobject obj=env->NewObject(cls,constructor);
      jfieldID fld=env->GetFieldID(cls,"register","Ljava/lang/Object;");
      env->SetObjectField(obj,fld,reinterpret_cast<jobject>(new CppType(x)));
      return obj;
    }
    template <class CppType>
    static jobject from(JNIEnv *env, CppType& x)
    {
      jclass cls=env->FindClass("ConcreteCppObject");
      jmethodID constructor=env->GetMethodID(cls,"<init>","()V");
      jobject obj=env->NewObject(cls,constructor);
      jfieldID fld=env->GetFieldID(cls,"register","Ljava/lang/Object;");
      env->SetObjectField(obj,fld,reinterpret_cast<jobject>(&x));
      return obj;
    }
  };

  /// specialisation for strings
  template <>
  struct JNItype<jstring>
  {
    static jstring from(JNIEnv *env, const char* x) 
    {
      std::vector<jchar> tmp(x, x+strlen(x));
      return env->NewString(&tmp[0], strlen(x));
    }
    static jstring from(JNIEnv *env, const std::string& x) 
    {
      return from(env, x.c_str());
    }
  };
}

#ifdef _CLASSDESC
#pragma omit pack classdesc::Arg
#pragma omit unpack classdesc::Arg
#endif

namespace classdesc_access
{
  //no sensible serialisation for this
  template <>
  struct access_pack<classdesc::Arg>
  {
    void operator()(classdesc::pack_t& t, const classdesc::string& d, classdesc::Arg& a) {} 
  };

  template <>
  struct access_unpack<classdesc::Arg>
  {
    void operator()(classdesc::pack_t& t, const classdesc::string& d, classdesc::Arg& a) {} 
  };
}

namespace classdesc
{
  /**
     A vector of arguments that initialises from a jobjectArray, and
     modifies said object array if any of the arguments return a changed value
  */
  class ArgVector: public std::vector<Arg>
  {
  public:
    ArgVector(size_t n=0): std::vector<Arg>(n) {}
    ArgVector(JNIEnv* env, jobjectArray& args)
    {
      for (size_t i=0; args && i<size_t(env->GetArrayLength(args)); ++i)
        push_back(classdesc::Arg(env, env->GetObjectArrayElement(args,i)));
    }
  };

  struct Functional_base: object
  {
    virtual void operator()(RetVal& ret, ArgVector& args)=0;
  };

  typedef poly<Functional_base> Functional_ptr;

  /**
     used for storing method pointers and other functionals in the 
     method_registry
  */
  template <class F>
  class Functional: public Object<Functional<F>, Functional_base>
  {
    F f;
public:
    Functional(const F& f): f(f) {}
    void operator()(RetVal& r, ArgVector& args) {
      apply(r.getRef<typename functional::Return<F>::T>(), f, args);}
  };
}

#ifdef _CLASSDESC
#pragma omit pack classdesc::Functional
#pragma omit unpack classdesc::Functional
#endif

namespace classdesc_access
{
  //no sensible serialisation for this
  template <class F>
  struct access_pack<classdesc::Functional<F> >: 
    public classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class F>
  struct access_unpack<classdesc::Functional<F> >:
    public classdesc::NullDescriptor<classdesc::unpack_t> {};

}

namespace classdesc
{
 struct javaClass_t: public ClassFile
  {
    javaClass_t() {magic=0xCAFEBABE; minor_version=0; major_version=50;}
    // add a functor to the method database
    virtual void add_functor(const std::string& name, Functional_ptr f, 
                             const std::string& sig)
    {}
  };
  
#ifdef _CLASSDESC
#pragma omit javaClass classdesc::javaClass_t
#endif

  template <class C, class M>
  typename enable_if<is_member_function_pointer<M>, void>::T
  javaClass(javaClass_t& cl, const string& desc, C& obj, M mem)
  {
    using namespace classdesc;
    std::string method_name(desc);
    if (method_name.find('.')!=std::string::npos)
      method_name=method_name.substr(method_name.rfind('.')+1);
    cl.addMethod(method_name, descriptor<M>());
//    cl.addMethod(method_name, std::string("([Ljava/lang/Object;)")+
//                  descriptor<typename functional::Return<M>::T>());
    cl.add_functor(std::string(desc),
                    Functional_ptr().addObject<Functional<
                       functional::bound_method<C,M> > >
                    (functional::bound_method<C,M>(obj,mem)),
                    descriptor<M>()
                    );
  }

  template <class T, class B>
  void javaClass(javaClass_t& cl, const string& desc, Object<T,B>& obj)
  {
    //TODO: maybe we want to add some default object properties here
  }

  template <class T>
  class getter_setter: public Object<getter_setter<T>,Functional_base>
  {
    T& m;
  public:
    getter_setter(T& m): m(m) {}
    void operator()(RetVal& r, ArgVector& args) {
      if (args.size()) m=args[0].get<T>(); *r.getRef<T>()=m;
    }
  };
}

#ifdef _CLASSDESC
#pragma omit pack classdesc::getter_setter
#pragma omit unpack classdesc::getter_setter
#pragma omit javaClass classdesc::object
#pragma omit javaClass classdesc::Object
#endif

namespace classdesc_access
{
  //no sensible serialisation for this
  template <class F>
  struct access_pack<classdesc::getter_setter<F> >:
    classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class F>
  struct access_unpack<classdesc::getter_setter<F> >:
    classdesc::NullDescriptor<classdesc::unpack_t> {};
}

namespace classdesc  
{
  // handle data members
  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  javaClass(javaClass_t& cl, const string& desc, T& obj, dummy<1> d=0)
  {
    std::string desc1(desc), ivar_name, class_name;
    std::string::size_type last_dot=desc1.rfind('.');
    
    if (last_dot==std::string::npos)
      ivar_name=desc1;
    else
      {
        ivar_name=desc1.substr(last_dot+1);
        class_name=desc1.substr(0,last_dot);
      }
    cl.addMethod(ivar_name, classdesc::descriptor<T (*)(T)>());
    cl.add_functor(desc1, getter_setter<T>(obj), descriptor<T (*)(T)>()); 

    //capitalise for java getter/setter conventions
    ivar_name[0]=toupper(ivar_name[0]); 
    cl.addMethod(std::string("get")+ivar_name, descriptor<T (*)()>());
    cl.add_functor(class_name+".get"+ivar_name, getter_setter<T>(obj), descriptor<T (*)()>()); 
    cl.addMethod(std::string("set")+ivar_name, descriptor<void (*)(T)>());
    cl.add_functor(class_name+".set"+ivar_name, getter_setter<T>(obj), descriptor<void (*)(T)>()); 
}

  // helper class to indicate specific javaClass not provided in this file
  template <class T>
  struct is_leftOver {static const bool value=!is_fundamental<T>::value;};

  template <class T>
  typename enable_if< is_leftOver<T>, void>::T
  javaClass(javaClass_t& cl, const string& desc, T& arg, dummy<0> d=0);

  template <class T>
  void javaClass(javaClass_t& cl,const string& d, const T& a)
  {javaClass(cl,d,const_cast<T&>(a));}

  template <class T>
  void javaClass(javaClass_t& cl,const string& d, is_const_static i, T a)
  {
    T tmp(a);
    javaClass(cl,d,tmp);
  }

  template <class T>
  void javaClass_onbase(javaClass_t& cl,const string& d,T a)
  {javaClass(cl,d,a);}

}  
  // const members, just discard constness as we're just doing descriptors
//  template <class C, class R>
//  void javaClass(javaClass_t& cl, const string& desc, C& obj, R (C::*mem)() const)
//  {
//    typedef R (C::*NonConstMember)();
//    javaClass(cl,desc,obj,NonConstMember(0));
//  }

//  template <class C, class R>
//  void javaClass(javaClass_t& cl, const string& desc, C& obj, R (C::*mem)())
//  {
//    method_info mi;
//    mi.access_flags=JVM_ACC_PUBLIC;
//    mi.name_index=cl->constant_pool.size();
//    cl->constant_pool.push_back(
//                  cp_info(JVM_CONSTANT_Utf8, std::string(desc)));
//    mi.descriptor_index=cl->constant_pool.size();
//    cl->constant_pool.push_back(
//                  cp_info(JVM_CONSTANT_Utf8, descriptor<R (C::*)()>()));
//    cl->methods.push_back(mi);
//  }

  //ignore anything we don't know
namespace classdesc_access
{
  template <class T> struct access_javaClass
  {
    void operator()(classdesc::javaClass_t& cl, const classdesc::string& desc, T& arg) {} 
  };
}



using classdesc::javaClass;

#endif
