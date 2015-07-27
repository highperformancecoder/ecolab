/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef CLASSDESC_H
#define CLASSDESC_H

// ensure link time failure if classdesc_epilogue.h not included
namespace
{
  int classdesc_epilogue_not_included();
  int _dummy=classdesc_epilogue_not_included();
}

#include <string>
#include <sstream>
#include <map>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <memory>
#include <climits>

#ifdef __CYGWIN__
namespace std
{
  //missing in Cygwin?
  typedef basic_string<wchar_t> wstring;
}
#endif

#ifndef NO_FWD_DECLARE_STL
namespace std
{
  // forward decls to help with conatiner type_traits, avoiding
  // unecessary #includes of STL headers
  template <class T, class A> class vector;
  template <class T, class A> class deque;
  template <class T, class A> class list;
  template <class T, class C, class A> class set;
  template <class K, class V, class C, class A> class map;
  template <class T, class C, class A> class multiset;
  template <class K, class V, class C, class A> class multimap;
#if defined(__cplusplus) && __cplusplus>=201103L
  template <class K, class V, class H, class P, class A> class unordered_map;
  template <class K, class H, class P, class A> class unordered_set;
  template <class K, class V, class H, class P, class A> class unordered_multimap;
  template <class K, class H, class P, class A> class unordered_multiset;
#endif
}
#else // for some reason the above decls dont work on MINGW
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>
#if defined(__cplusplus) && __cplusplus>=201103L
#include <unordered_map>
#include <unordered_set>
#endif
#endif


  /*
    Classdesc is now dependent on TR1. These can either
    be obtained from the compiler's standard library, or from Boost.
    TODO: check that this works with Boost!
  */
#ifdef TR1
#include <tr1/type_traits>
#if !defined(__ICC) || __ICC > 1100    //tr1 shared_ptr impl not functional with icc 10.1
#include <tr1/memory>
#endif
#elif BOOST_TR1
#include <boost/tr1/type_traits.hpp>
#include <boost/tr1/memory.hpp>
#endif

/** \namespace classdesc \brief Contains definitions related to
    classdesc functionality
*/

#if defined(__cplusplus) && __cplusplus>=201103L

#include <unordered_map>
#include <unordered_set>

#include <type_traits>
namespace classdesc
{
  using std::true_type;
  using std::false_type;

  using std::is_void;  
  using std::is_integral;
  using std::is_floating_point;
  // is_arry conflicts with an already established classdesc concept
  //  using std::is_array; 
  using std::is_pointer;
  using std::is_reference;
  using std::is_member_object_pointer;
  using std::is_member_function_pointer;
  using std::is_enum;
  using std::is_union;
  using std::is_class;
  using std::is_function;

  using std::is_arithmetic;
  using std::is_fundamental;
  using std::is_object;
  using std::is_scalar;
  using std::is_compound;
  using std::is_member_pointer;

  using std::is_const;
  using std::is_volatile;
  using std::is_pod;
  using std::is_empty;
  using std::is_polymorphic;
  using std::is_abstract;
  using std::is_signed;
  using std::is_unsigned;
  using std::alignment_of;
  // conflicts with ecolab definition
  //  using std::rank;
  using std::extent;

  using std::is_same;
  using std::is_base_of;
  using std::is_convertible;

  using std::remove_const;
  using std::remove_volatile;
  using std::remove_cv;
  using std::add_const;
  using std::add_volatile;
  using std::add_cv;

  using std::remove_reference;

  using std::remove_extent;
  using std::remove_all_extents;

  using std::remove_pointer;
  using std::add_pointer;


  using std::shared_ptr;
  using std::weak_ptr;

  using std::is_default_constructible;
  using std::is_copy_constructible;
  using std::is_assignable;
}

// long long is now part of the standard language
#ifndef HAVE_LONGLONG
#define HAVE_LONGLONG
#endif

#elif defined(BOOST_TR1) || defined(TR1) || defined(_MSC_VER) 
namespace classdesc
{
  using std::tr1::true_type;
  using std::tr1::false_type;

  using std::tr1::is_void;  
  using std::tr1::is_integral;
  using std::tr1::is_floating_point;
  // conflicts with an already established classdesc concept
  //using std::tr1::is_array;
  using std::tr1::is_pointer;
  using std::tr1::is_reference;
  using std::tr1::is_member_object_pointer;
  using std::tr1::is_member_function_pointer;
  using std::tr1::is_enum;
  using std::tr1::is_union;
  using std::tr1::is_class;
  using std::tr1::is_function;

  using std::tr1::is_arithmetic;
  using std::tr1::is_fundamental;
  using std::tr1::is_object;
  using std::tr1::is_scalar;
  using std::tr1::is_compound;
  using std::tr1::is_member_pointer;

  using std::tr1::is_const;
  using std::tr1::is_volatile;
  using std::tr1::is_pod;
  using std::tr1::is_empty;
  using std::tr1::is_polymorphic;
  using std::tr1::is_abstract;
  using std::tr1::is_signed;
  using std::tr1::is_unsigned;
  using std::tr1::alignment_of;
  // conflicts with ecolab definition
  //  using std::tr1::rank;
  using std::tr1::extent;

  using std::tr1::is_same;
  using std::tr1::is_base_of;
  using std::tr1::is_convertible;

  using std::tr1::remove_const;
  using std::tr1::remove_volatile;
  using std::tr1::remove_cv;
  using std::tr1::add_const;
  using std::tr1::add_volatile;
  using std::tr1::add_cv;

  using std::tr1::remove_reference;

  using std::tr1::remove_extent;
  using std::tr1::remove_all_extents;

  using std::tr1::remove_pointer;
  using std::tr1::add_pointer;

  // nb ultimately, all compilers will support std::shared_ptr, but
  // until then, this bit of ugly hackery is required.
#if (!defined(__ICC) || __ICC > 1100)
  using std::tr1::shared_ptr;
#endif

  // fake these using TR1 counterparts (which are conservative)
  template <class T> struct is_default_constructible:
    public std::tr1::has_nothrow_constructor<T> {};
  template <class T> struct is_copy_constructible:
    public std::tr1::has_nothrow_copy<T> {};
  template <class T, class U> struct is_assignable
  {
    static const bool value=std::tr1::has_nothrow_assign<T>::value &&
      std::tr1::is_convertible<U,T>::value;
  };
 
  // ensure at least strings work!
  template <class C, class A> 
  struct is_default_constructible<std::basic_string<C,A> >: 
    public std::tr1::true_type {};
  template <class C, class A> 
  struct is_copy_constructible<std::basic_string<C,A> >: 
    public std::tr1::true_type {};
  template <class C, class A, class U> 
  struct is_assignable<std::basic_string<C,A>, U>
  {
    static const bool value=
      std::tr1::is_convertible<U,std::basic_string<C,A> >::value;
  };
}  

#endif


namespace classdesc
{
  using std::string;
  template <bool, class type=void> struct enable_if_c {typedef type T;};
  template <class T> struct enable_if_c<false,T> {};
  /// controlled template specialisation: stolen from boost::enable_if. 
  /** 
  \a Cond is a condition class inheriting from \c
     std::tr1::true_type or std::tr1::false_type
  */
  template <class Cond, class T=void> struct enable_if: 
    public enable_if_c<Cond::value,T> {};
      
  // NB - implementation of C++11 std::conditional
  template <bool C, class True, class F>
  struct conditional
  {
    typedef True T;
  };
  
  template <class True, class F>
  struct conditional<false, True, F>
  {
    typedef F T;
  };

  // to help distinguish functions templates on old compilers (eg gcc 3.2)
  template <int> struct dummy {dummy(int) {} };

  /// @{ determines if T is a standard sequence container
  template <class T> struct is_sequence {static const bool value=false;};
  template <class T, class A> struct is_sequence<std::vector<T,A> > {
    static const bool value=true;};
  template <class T, class A> struct is_sequence<std::deque<T,A> > {
    static const bool value=true;};
  template <class T, class A> struct is_sequence<std::list<T,A> > {
    static const bool value=true;};
  template <class T> struct is_sequence<const T>: public is_sequence<T> {};
  /// @}

  /// @{ determines if this is a string
  template <class T> struct is_string {static const bool value=false;};
  template <class T> struct is_string<std::basic_string<T> >
  {static const bool value=true;};
  template <class T> struct is_string<const T>: public is_string<T> {};
  /// @}

  /// determines if T is a standard associative container
  template <class T> 
  struct is_associative_container {static const bool value=false;};
  template <class T, class C, class A> 
  struct is_associative_container<std::set<T,C,A> > {
    static const bool value=true;};
  template <class K, class V, class C, class A> 
  struct is_associative_container<std::map<K,V,C,A> > {
    static const bool value=true;};
   template <class T, class C, class A> 
  struct is_associative_container<std::multiset<T,C,A> > {
    static const bool value=true;};
  template <class K, class V, class C, class A> 
  struct is_associative_container<std::multimap<K,V,C,A> > {
    static const bool value=true;};
 
#if defined(__cplusplus) && __cplusplus>=201103L
  template <class K, class V, class H, class P, class A> 
  struct is_associative_container<std::unordered_map<K,V,H,P,A> > {
    static const bool value=true;};
  template <class K, class H, class P, class A> 
  struct is_associative_container<std::unordered_set<K,H,P,A> > {
    static const bool value=true;};
  template <class K, class V, class H, class P, class A> 
  struct is_associative_container<std::unordered_multimap<K,V,H,P,A> > {
    static const bool value=true;};
  template <class K, class H, class P, class A> 
  struct is_associative_container<std::unordered_multiset<K,H,P,A> > {
    static const bool value=true;};
#endif
  template <class T> struct is_associative_container<const T>: public is_associative_container<T> {};

  /// determines if T is a container
  template <class T> struct is_container {
    static const bool value=
    is_sequence<T>::value||is_associative_container<T>::value;
  };

  ///  boolean arithmetic on is_ structs 
  ///@{
  template <class T> struct Not
  {static const bool value=!T::value;};

  template <class A, class B> struct And
  {static const bool value=A::value && B::value;};

  template <class A, class B> struct Or
  {static const bool value=A::value || B::value;};

  template <int X, int Y> struct Eq
  {static const bool value=X==Y;};

  ///@}

  /// has default constructor, and is copiable 

  // is_assignable doesn't seem to be working correctly yet
//  template <class T> struct is_dca: 
//    public And<And<is_default_constructible<T>, is_copy_constructible<T> >,
//               is_assignable<T,T> > {};
  template <class T> struct is_dca: 
    public And<is_default_constructible<T>, is_copy_constructible<T> > {};


  /// @{can a temporary of type T be constructed and passed to an argument
  template <class T> struct is_rvalue
  {
    static const bool value=is_dca<T>::value && !is_abstract<T>::value;
  };

  template <class T> struct is_rvalue<T&>
  {
    static const bool value=false;
  };

  template <class T> struct is_rvalue<const T&>: public is_rvalue<T> {};

    /// @}
    
  /// base class for exceptions thrown by classdesc
  struct exception: std::runtime_error
  {
    exception(const string& s="classdesc exception"): std::runtime_error(s) {}
  };


  /*
    Support for typeName functionality
  */
  template <class T> struct tn;  //for partial specialisation support
  template <class T> std::string typeName();

  ///@{ a string representation of the type
  template <> inline std::string typeName<bool>()    {return "bool";}
  template <> inline std::string typeName<char>()    {return "char";}
  template <> inline std::string typeName<short>() {return "short";}
  template <> inline std::string typeName<int>()   {return "int";}
  template <> inline std::string typeName<long>()  {return "long";}

  template <> inline std::string typeName<signed char>()   {return "signed char";}
  template <> inline std::string typeName<unsigned char>() {return "unsigned char";}
  template <> inline std::string typeName<unsigned short>(){return "unsigned short";}
  template <> inline std::string typeName<unsigned int>()  {return "unsigned int";}
  template <> inline std::string typeName<unsigned long>() {return "unsigned long";}

#ifdef HAVE_LONGLONG
  template <> inline std::string typeName<long long>()                {return "long long";}
  template <> inline std::string typeName<unsigned long long>()       {return "unsigned long long";}
#endif

  template <> inline std::string typeName<float>()       {return "float";}
  template <> inline std::string typeName<double>()      {return "double";}
  template <> inline std::string typeName<long double>() {return "long double";}


  template <> inline std::string typeName<std::string>()        {return "std::string";}
  template <> inline std::string typeName<std::wstring>()       {return "std::wstring";}
  ///@}

  // handle C++ new fixed width types
#if __cplusplus>=201103L
  template <> inline std::string typeName<char16_t>()    {return "char16_t";}
  template <> inline std::string typeName<char32_t>()    {return "char32_t";}
#endif

  template <class T> struct tn<T*>
  {
    static std::string name()
    {return typeName<T>()+"*";}
  };

  template <class T,class A> struct tn<std::vector<T,A> >
  {
    static std::string name()
    {return "std::vector<"+typeName<T>()+">";}
  };

  template <class T, class A> struct tn<std::list<T,A> >
  {
    static std::string name()
    {return "std::list<"+typeName<T>()+">";}
  };

  template <class T,class A> struct tn<std::deque<T,A> >
  {
    static std::string name()
    {return "std::deque<"+typeName<T>()+">";}
  };

  template <class T, class C, class A> struct tn<std::set<T,C,A> >
  {
    static std::string name()
    {return "std::set<"+typeName<T>()+">";}
  };

  template <class K, class V, class C, class A> struct tn<std::map<K,V,C,A> >
  {
    static std::string name()
    {return "std::map<"+typeName<K>()+","+typeName<V>()+">";}
  };

  template <class K, class V> struct tn<std::pair<K,V> >
  {
    static std::string name()
    {return "std::pair<"+typeName<K>()+","+typeName<V>()+">";}
  };



  /// enum symbol handling
  struct EnumKey
  {
    const char* name;
    int value;
  };

  typedef std::map<std::string,int> SVMap;
  typedef std::map<int,std::string> VSMap;

  template <class T>
  class EnumKeys
  {
    SVMap s2v; ///< map enum symbol to value
    VSMap v2s; ///< map enum value to symbol
  public:
    EnumKeys(const EnumKey* data, int size)
    {
      for (const EnumKey *i=data; i<data+size; i++)
        {
          s2v[i->name]=i->value;
          v2s[i->value]=i->name;
        }
    }
    T operator()(std::string key) const {
      SVMap::const_iterator i=s2v.find(key);
      if (i!=s2v.end()) return T(i->second);
      else return T(0);
    }
    std::string operator()(int val) const 
    {
      VSMap::const_iterator i=v2s.find(val);
      if (i!=v2s.end()) return i->second;
      else return "";
    }
    // use these to iterate of the enum's keys
    size_t size() const {return v2s.size();}
    typedef VSMap::const_iterator iterator;
    iterator begin() const {return v2s.begin();}
    iterator end() const {return v2s.end();}

    typedef iterator It; // resolves a type ambiguity below
    class Siterator: public It
    {
    public:
      typedef string value_type;
      typedef string* pointer;
      typedef const string& reference;
      Siterator() {}
      Siterator(const It& i): It(i) {}
      const string& operator*() const {return It::operator*().second;}
      const string* operator->() const {return &It::operator*().second;}
    };

    Siterator sbegin() const {return begin();}
    Siterator send() const {return end();}
    
    class Viterator: public It
    {
    public:
      typedef T value_type;
      typedef T* pointer;
      typedef const T& reference;
      Viterator() {}
      Viterator(const It& i): It(i) {}
      T operator*() const {return T(It::operator*().first);}
    };

    Viterator vbegin() const {return begin();}
    Viterator vend() const {return end();}
  };

  namespace {
    template <class T> 
    struct enum_keysData
    {
      static EnumKey keysData[];
      static EnumKeys<T> keys;
    };

    template <class T> int enumKey(const std::string&);
    template <class T> std::string enumKey(int);
  }

  template <class T> const EnumKeys<T>&
  enum_keys() {return enum_keysData<T>::keys;}

  /**
     Enum_handle is a wrapper class that performs serialisation
     of enums using symbolic constants
  */
  template <class T> //T is an enum
  class Enum_handle
  {
    T& ref;
  public:
    Enum_handle(T& arg): ref(arg) {}
    operator std::string() const {
      return enumKey<typename remove_const<T>::type>(static_cast<int>(ref));
    }
    operator int() const {return static_cast<int>(ref);}
    const Enum_handle& operator=(T x) {ref=x; return *this;}
    const Enum_handle& operator=(int x) {ref=T(x); return *this;}
    const Enum_handle& operator=(const std::string& x) 
    {ref=T(enumKey<T>(x)); return *this;}
  };

  template <class T>
  std::istream& operator>>(std::istream& i, Enum_handle<T>& x)
  {
    std::string temp;
    i>>temp;
    x=temp;
    return i;
  }

  template <class T>
  std::ostream& operator<<(std::ostream& o, Enum_handle<T> x)
  {
    o << static_cast<std::string>(x);
    return o;
  }

  template <class T>
  Enum_handle<T> enum_handle(T& x) {return Enum_handle<T>(x);}

  /** support for constant sized arrays  */
  class is_array {};

  /** support for const static members (which needn't have references) */
  class is_const_static {};

  /** support for graph structures */
  class is_node {};
  class is_treenode: public is_node {};
  class is_graphnode: public is_node {};

  // derive from this to create a null descriptor
  template <class T> 
  struct dummy_functional
  {
    void operator()(void *targ, const string& desc, T& arg) {}
  };

  // get last component of name
  inline std::string tail(const string& x) {
    std::string r(x);
    std::string::size_type i=r.rfind('.');
    return r.substr( i==std::string::npos? 0: i+1);
  }

  // get all but last component of name
  inline std::string head(const string& x) {
    std::string r(x);
    std::string::size_type i=r.rfind('.');
    return r.substr( 0, i==std::string::npos? std::string::npos: i);
  }

  /// helper for unpacking into map value_types
  template <class TT> struct NonConstKeyValueType
  {typedef TT T;};

  template <class K, class V> struct NonConstKeyValueType<std::pair<const K,V> >
  {typedef std::pair<K,V> T;};


  /// @{ Exclude marker (for use in preferentially excluding descriptors)
  /// If you have a member XXX xxx, then declaring it as Exclude<XXX>
  /// xxx excludes the variable from classdesc description (ie is not
  /// serialised)
  template <class T>
  struct ExcludeClass: public T
  {
    ExcludeClass() {}
    template <class U> explicit ExcludeClass(const U& x): T(x) {}
    template <class U> const T& operator=(const U& x) {return T::operator=(x);}
    template <class U> operator const U&() const {return *static_cast<U*>(this);}
    template <class U> operator U&() {return *static_cast<U*>(this);}
    // other operators?
  };

  template <class T>
  struct ExcludeFundamental
  {                           
    typedef T& RefType;    
    typedef const T& ConstRefType;  
    T val;                          
    ExcludeFundamental() {}                       
    template <class U> explicit ExcludeFundamental(const U& x): val(x) {}
    template <class U> const T& operator=(const U& x) {return val=x;}
    template <class U> operator U() const {return val;}   
    operator RefType () {return val;}                     
    operator ConstRefType () const {return val;}          
    T operator+(const T& x) const {return val+x;}   
    T operator-(const T& x) const {return val-x;}   
    T operator*(const T& x) const {return val*x;}  
    T operator/(const T& x) const {return val/x;}  
    T operator%(const T& x) const {return mod(val,x);} 
    T operator+=(const T& x) {return val+=x;}          
    T operator-=(const T& x) {return val-=x;}          
    T operator*=(const T& x) {return val*=x;}          
    T operator/=(const T& x) {return val/=x;}          
    T operator%=(const T& x) {val=mod(val,x); return val;}
    bool operator==(const T& x) const {return val==x;}
    bool operator==(const ExcludeFundamental<T>& x) const {return val==x.val;}
    template <class U> bool operator!=(U x) const {return !operator==(x);}
  };                                               

  template <class T> struct Exclude: public
  conditional<is_class<T>::value, ExcludeClass<T>, ExcludeFundamental<T> >::T
  {
    typedef typename conditional
    <is_class<T>::value, ExcludeClass<T>, ExcludeFundamental<T> >::T Super;
    Exclude() {}
    template <class U> explicit Exclude(const U& x): Super(x) {}  
    template <class U> const T& operator=(const U& x) {return Super::operator=(x);}
  };

  template <class T>                                   
  struct Exclude<T*>                         
  {                                             
    T* val;                                  
    Exclude(): val(NULL) {} 
#if __cplusplus>=201103L                    
    Exclude(std::nullptr_t): val(nullptr) {}
    bool operator==(std::nullptr_t) const {return val==nullptr;}
#endif
    bool operator==(const T* x) const {return val==x;}
    bool operator==(const Exclude<T*>& x) const {return val==x.val;}
    template <class U> bool operator!=(U x) const {return !operator==(x);}
    template <class U> explicit Exclude(const U& x): val(x) {}  
    template <class U> const T& operator=(U x) {return *(val=x);} 
    template <class U> operator U() const {return val;}         
    T& operator*() {return *val;}
    const T& operator*() const {return *val;}
    T* operator->() {return val;}
    const T* operator->() const {return val;} 
    template <class U>
    typename enable_if<is_integral<U>,T*>::T
    operator+(U x) {return val+x;}
    template <class U>
    typename enable_if<is_integral<U>,const T*>::T
    operator+(U x) const {return val+x;}
    std::ptrdiff_t operator-(const T* x) const {return val-x;}
 };

  template <class T, class U>
  typename enable_if<is_integral<U>,T*>::T
  operator+(U x, Exclude<T*> y)
  {return y+x;}

  template <class T>
  std::ptrdiff_t operator-(const T* x, const Exclude<T*> y)
  {return -(y-x);}

  template <class T>
  std::ptrdiff_t operator-(T* x, const Exclude<T*> y)
  {return -(y-x);}

  template <class T, class U> T mod(T x, U y) {return x%y;}
  template <class U> float mod(float x, U y) {return std::fmod(x,y);}
  template <class U> double mod(double x, U y) {return std::fmod(x,y);}


  // handle fundamental types
  template <>
  struct Exclude<bool>
  {          
    typedef bool& RefType;                                             
    typedef const bool& ConstRefType; 
    bool val;                               
    Exclude() {}                            
    template <class U> explicit Exclude(const U& x): val(x) {}  
    template <class U> bool operator=(const U& x) {return val=x;}
    template <class U> operator U() const {return val;}   
    operator RefType () {return val;}              
    operator ConstRefType () const {return val;} 
    bool operator&&(const bool& x) const {return val&&x;} 
    bool operator||(const bool& x) const {return val||x;} 
    bool operator&=(const bool& x) {return val=val&&x;} 
    bool operator|=(const bool& x) {return val=val||x;} 
    bool operator!() const {return !val;}  
  };                                               

  /// @}

  /// helper for constructing null descriptors
  template <class action_t>
  struct NullDescriptor
  {
    template <class U>
    void operator()(action_t&,const string&,U&) {}
  };

  /// @{
  /// casts to a base class type, preserving constness attributes
  
  template <class B>
  struct base_cast
  {
    template <class C> 
    static B& cast(C& x) {return static_cast<B&>(x);}
    template <class C> 
    static const B& cast(const C& x) {return static_cast<const B&>(x);}
  };
  /// @}

  /// returns a valid identifier to append to the descriptor of a base class
  template <class T> string basename()
  {
    string r(".base_"+typeName<T>());
    for (size_t i=1; i<r.size(); ++i)
      if (!isalnum(r[i])) r[i]='_';
    return r;
  }



}
#endif
