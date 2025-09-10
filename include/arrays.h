/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief dynamic array class
*/
#ifndef ARRAYS_H
#define ARRAYS_H

#include "random.h"
#include "classdesc.h"
#include <assert.h>
#include <cmath>
#include <limits>

#include <set>
#include <vector>
#include <iostream>
#include <iterator>

#ifdef MKL
#ifdef MKL_STUB
#include "mkl_stub.h"
#else
#include <mkl.h>
#endif
#endif

#include <pack_base.h>

#include "ecolab.h"

#ifdef SYCL_LANGUAGE_VERSION
#ifdef __INTEL_LLVM_COMPILER
#include <sycl/ext/oneapi/group_local_memory.hpp>
#endif
#endif

namespace ecolab
{

  namespace array_ns
  {

    /** \namespace array_ns
        \brief Namespace for array functionality.

        Most methods can take an \em expression template as an argument. An
        \em expression template has the following properties - 
        size(), operator[] and value_type

        NB do not dump this namespace into global namespace. That is a
        recipe for very strange compiler errors. Instead selectively
        import what you need. Koenig lookup means most functionality is
        automatically exported when needed.
    */

    template <class T, class A> class array;
    template <class E, class Op> class unop;
    template <class E1, class E2, class Op>  class binop;
    template <class E, class I> class RVindex;
    template <class E, class I> class LVindex;
  
    /*
      type traits classes for testing whether a template argument is an
      expression - an array, a unary operator on an expression, or a
      binary operator with at least one argument being an expression
    */

    template <class T> struct is_expression
    {static const bool value=false;};

    template <class T,class A> struct is_expression<array<T,A> >
    {static const bool value=true;};
  
    template <class T, class Op> struct is_expression<unop<T,Op> >
    {static const bool value=true;};
  
    template <class E1, class E2, class Op> struct is_expression<binop<E1,E2,Op> >
    {static const bool value=true;};

    /* is true if at least one argument is an expression */
    template <class E1, class E2> struct one_is_expression
    {static const bool value=is_expression<E1>::value || is_expression<E2>::value;};

    /* is true if both arguments are expressions */
    template <class E1, class E2> struct both_are_expressions
    {static const bool value=is_expression<E1>::value && is_expression<E2>::value;};
  
    /* type traits class indicating if type is simple data type */
    template <class T> struct is_scalar {static const bool value=false;};
      
    template <> struct is_scalar<bool> {static const bool value=true;};
    template <> struct is_scalar<char> {static const bool value=true;};
    template <> struct is_scalar<short> {static const bool value=true;};
    template <> struct is_scalar<int> {static const bool value=true;};
    template <> struct is_scalar<long> {static const bool value=true;};
    template <> struct is_scalar<unsigned char> {static const bool value=true;};
    template <> struct is_scalar<unsigned short> {static const bool value=true;};
    template <> struct is_scalar<unsigned int> {static const bool value=true;};
    template <> struct is_scalar<unsigned long> {static const bool value=true;};
    template <> struct is_scalar<float> {static const bool value=true;};
    template <> struct is_scalar<double> {static const bool value=true;};
      
    /* type traits class indicating if type is integral */
    template <class T> struct is_integer {static const bool value=false;};
    template <> struct is_integer<bool> {static const bool value=true;};
    template <> struct is_integer<char> {static const bool value=true;};
    template <> struct is_integer<short> {static const bool value=true;};
    template <> struct is_integer<int> {static const bool value=true;};
    template <> struct is_integer<long> {static const bool value=true;};
    template <> struct is_integer<unsigned char> {static const bool value=true;};
    template <> struct is_integer<unsigned short> {static const bool value=true;};
    template <> struct is_integer<unsigned int> {static const bool value=true;};
    template <> struct is_integer<unsigned long> {static const bool value=true;};

#ifdef HAVE_LONGLONG
    template <> struct is_scalar<long long> {static const bool value=true;};
    template <> struct is_scalar<unsigned long long> {static const bool value=true;};
    template <> struct is_integer<long long> {static const bool value=true;};
    template <> struct is_integer<unsigned long long> {static const bool value=true;};
#endif
  
    /* type traits class indicating if type is floating point */
    template <class T> struct is_float {static const bool value=false;};
    template <> struct is_float<float> {static const bool value=true;};
    template <> struct is_float<double> {static const bool value=true;};

    template <class E1, class E2> struct is_expression_and_scalar
    {static const bool value=is_expression<E1>::value && is_scalar<E2>::value;};

    /// true if both E1 and E2 are expressions, or one is and the other a scalar
    template <class E1, class E2> struct is_expression_or_scalar:
      public classdesc::Or<
      both_are_expressions<E1,E2>,
      is_expression_and_scalar<E1,E2>,
      is_expression_and_scalar<E2,E1>
      > {};

    template <bool, class type=void> struct enable_if_c {typedef type T;};
    template <class T> struct enable_if_c<false,T> {};
    /* used for controlled template specialisation: stolen from boost::enable_if */
    template <class Cond, class T=void> struct enable_if: 
      public enable_if_c<Cond::value,T> {};
      
    // to help distinguish functions templates on old compilers (eg gcc 3.2)
    template <int> struct dummy {dummy(int) {} };

    template <class U, class=void> struct Allocator {using type=void; using T=type;};
    template <class E> struct Allocator<E, classdesc::void_t<typename E::Allocator>>
    {using type=typename E::Allocator; using T=type;};

    // return an allocator type for type T, based on an allocator for a different type
    template <class T, template<class> class A, class U>
    A<T> makeAllocator(const A<U>& a) {return A<T>(a);}

    template <class T, class U> struct MakeAllocator;
    template <class T, template<class> class A, class U>
    struct MakeAllocator<T,A<U>>
    {
      using type=A<T>;
    };
    
    /*
      \c type_traits<T>::value_type return \c T::value_type is \a T is an 
      \em expression, or \c T if \a T is scalar
    */
  
    /* traits template for non-scalar, non-expressions */
    template <class T, bool=false, bool=false>
    struct tt
    {
      typedef void value_type;
    };

    /* traits template for expressions */
    template <class E>
    struct tt<E,true,false>
    {
      typedef typename E::value_type value_type;
    };

    /* traits template for scalars */
    template <class S>
    struct tt<S,false,true>
    {
      typedef S value_type;
    };

    /* combine the two */
    template <class T>
    struct traits: public tt<T,is_expression<T>::value,is_scalar<T>::value> {};

    /*
      result type of a binary expression T + U
    */
    template <class E1, class E2> 
    struct result  
    {
      typedef typename result<
        typename traits<E1>::value_type,
        typename traits<E2>::value_type
        >::value_type
      value_type;
    };

    template <class T> struct result<void,T> {typedef void value_type;};
    template <class T> struct result<T,void> {typedef void value_type;};
    template <> struct result<void,void> {typedef void value_type;};

    /*
      result type of integer/integer binary ops
    */
#define RESULTIII(T1,T2,R)                                              \
    template <> struct result<T1,T2> {typedef R value_type;};           \
    template <> struct result<T1,unsigned T2> {typedef R value_type;};  \
    template <> struct result<unsigned T1, T2> {typedef R value_type;}; \
    template <> struct result<unsigned T1,unsigned T2>{typedef unsigned R value_type;};

    /*
      result type of bool/integer binary ops
    */
#define RESULTBI(T1)                                                    \
    template <> struct result<T1,bool> {typedef T1 value_type;};        \
    template <> struct result<bool,T1> {typedef T1 value_type;};        \
    template <> struct result<bool,unsigned T1> {typedef unsigned T1 value_type;}; \
    template <> struct result<unsigned T1, bool> {typedef unsigned T1 value_type;}; \

    /*
      result type of bool/float binary ops
    */
#define RESULTBF(T1)                                                    \
    template <> struct result<T1,bool> {typedef T1 value_type;};        \
    template <> struct result<bool,T1> {typedef T1 value_type;};        \

    /*
      result type of integer/float binary ops
    */
#define RESULTIFF(T1,T2,R)                                              \
    template <> struct result<T1,T2> {typedef R value_type;};           \
    template <> struct result<unsigned T1, T2> {typedef R value_type;}; \

    /*
      result type of float/integer binary ops
    */
#define RESULTFIF(T1,T2,R)                                              \
    template <> struct result<T1,T2> {typedef R value_type;};           \
    template <> struct result<T1,unsigned T2> {typedef R value_type;};  \

    /*
      result type of float/float binary ops
    */
#define RESULTFFF(T1,T2,R)                                      \
    template <> struct result<T1,T2> {typedef R value_type;};

    template <> struct result<bool,bool> {typedef int value_type;};

    RESULTBI(char);
    RESULTBI(short);
    RESULTBI(int);
    RESULTBI(long);

    RESULTBF(float);
    RESULTBF(double);
    
    RESULTIII(char,char,char);
      
    RESULTIII(short,short,short);
    RESULTIII(char,short,short);
    RESULTIII(short,char,short);

    RESULTIII(int,int,int);
    RESULTIII(short,int,int);
    RESULTIII(int,short,int);
    RESULTIII(char,int,int);
    RESULTIII(int,char,int);

    RESULTIII(long,long,long);
    RESULTIII(int,long,long);
    RESULTIII(long,int,long);
    RESULTIII(short,long,long);
    RESULTIII(long,short,long);
    RESULTIII(char,long,long);
    RESULTIII(long,char,long);

#ifdef HAVE_LONGLONG
    RESULTIII(long long,long long,long long);
    RESULTIII(long,long long,long long);
    RESULTIII(long long,long,long long);
    RESULTIII(int,long long,long long);
    RESULTIII(long long,int,long long);
    RESULTIII(short,long long,long long);
    RESULTIII(long long,short,long long);
    RESULTIII(char,long long,long long);
    RESULTIII(long long,char,long long);
#endif
      
    RESULTFFF(float,float,float);
    RESULTIFF(long,float,float);
    RESULTFIF(float,long,float);
    RESULTIFF(int,float,float);
    RESULTFIF(float,int,float);
    RESULTIFF(short,float,float);
    RESULTFIF(float,short,float);
    RESULTIFF(char,float,float);
    RESULTFIF(float,char,float);

#ifdef _MSC_VER
    RESULTFIF(float,__int64,float);
    RESULTFIF(double,__int64,double);
#endif

    RESULTFFF(double,double,double);
    RESULTFFF(float,double,double);
    RESULTFFF(double,float,double);
    RESULTIFF(long,double,double);
    RESULTFIF(double,long,double);
    RESULTIFF(int,double,double);
    RESULTFIF(double,int,double);
    RESULTIFF(short,double,double);
    RESULTFIF(double,short,double);
    RESULTIFF(char,double,double);
    RESULTFIF(double,char,double);

#ifdef HAVE_LONGLONG
    RESULTIFF(long long,float,float);
    RESULTFIF(float,long long,float);
    RESULTIFF(long long,double,double);
    RESULTFIF(double,long long,double);
#endif

    template <class F>
    inline void map(size_t sz, F f) {
#ifdef  __SYCL_DEVICE_ONLY__
    // SYCL support for array operations over a group (SIMD compute unit)
      auto idx=syclGroup().get_local_linear_id();
      auto groupSz=syclGroup().get_local_linear_range();
      for (size_t i=idx; i<sz; i+=groupSz) f(i);
#else
#ifdef __ICC
#pragma loop count(1000)
#pragma ivdep
#pragma vector aligned
#endif
      for (size_t i=0; i<sz; ++i) f(i);
#endif      
   }
    
    /** at(x,i) = x[i] is an expression, otherwise just x */
    //@{
    template <class E> typename
    E::value_type& at(E& x, size_t i, 
                      typename enable_if< is_expression<E>, int >::T dum=0) 
    {return x[i];}

    template <class E> typename
    E::value_type at(const E& x, size_t i,
                     typename enable_if< is_expression<E>, int >::T dum=0) 
    {return x[i];}
  
    template <class S> 
    S& at(S& x, size_t i,
          typename enable_if< is_scalar<S>, int >::T dum=0) 
    {return x;}

    template <class S>
    S at(S x, size_t i,
         typename enable_if< is_scalar<S>, int >::T dum=0) 
    {return x;}
    //@}

    /** len(x)==x.size() if x is an expression, otherwise 1 */
    //@{
    template <class E> typename
    enable_if< is_expression<E>, size_t >::T
    len(const E& x, dummy<0> d=0) {return x.size();}
  
    template <class S> typename
    enable_if< is_scalar<S>, size_t >::T
    len(const S& x, dummy<1> d=0) {return 1;}

    /*
      \c conformance_check(e1,e2) is an assertion that e1 & e2 are conformant
      (same size), or one is scalar
    */
    template <class E1, class E2>
    void conformance_check(const E1& e1, const E2& e2)
    {
      assert(
             is_scalar<E1>::value || is_scalar<E2>::value || 
             len(e1)==len(e2)
             );
    }

    /* support class for elementwise negation */
    template <class T>
    struct Neg
    {
      typedef T value_type;
      T operator()(T x) const {return -x;}
    };
   
    /* support class for elementwise boolean not */

    template <class T>
    struct Not
    {
      typedef bool value_type;
      bool operator()(T x) const {return !x;}
    };
   
    /* support class for elementwise + */

    template <class E1, class E2>
    struct Add
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return x+y;}
    };

    /* support class for elementwise - */

    template <class E1, class E2>
    struct Sub
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return x-y;}
    };

    /* support class for elementwise * */

    template <class E1, class E2>
    struct Mul
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return x*y;}
    };


    /* support class for elementwise / */
    template <class E1, class E2>
    struct Div
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return x/y;}
    };

    /* support class for elementwise % */
    template <class E1, class E2>
    struct Mod
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return x%y;}
    };

    /* support class for elementwise == */
    template <class E1, class E2>
    struct Eq
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x==y;}
    };
      
    /* support class for elementwise != */
    template <class E1, class E2>
    struct Neq
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x!=y;}
    };
      
    /* support class for elementwise > */
    template <class E1, class E2>
    struct Gt
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x>y;}
    };

    /* support class for elementwise >= */
    template <class E1, class E2>
    struct Gte
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x>=y;}
    };

    /* support class for elementwise < */
    template <class E1, class E2>
    struct Lt
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x<y;}
    };

    /* support class for elementwise <= */
    template <class E1, class E2>
    struct Lte
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x<=y;}
    };
  
    /* support class for elementwise && */
    template <class E1, class E2>
    struct And
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x&&y;}
    };
      
    /* support class for elementwise || */
    template <class E1, class E2>
    struct Or
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef bool value_type;
      bool operator()(E1v x, E2v y) const {return x||y;}
    };
      
    template <class E>
    typename enable_if<is_expression<E>, unop<E,Neg<typename E::value_type> > >::T
    operator-(const E& e) {return  unop<E,Neg<typename E::value_type> >(e);}

    template <class E>
    typename enable_if<is_expression<E>, unop<E,Not<typename E::value_type> > >::T
    operator!(const E& e) {return  unop<E,Not<typename E::value_type> >(e);}

    template <class E1, class E2>
    typename enable_if<is_expression_or_scalar<E1,E2>, binop<E1,E2,Add<E1,E2> > >::T
    operator+(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Add<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if<is_expression_or_scalar<E1,E2>, binop<E1,E2,Sub<E1,E2> > >::T
    operator-(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Sub<E1,E2> >(e1,e2);
    }
    
    template <class E1, class E2>
    typename enable_if<is_expression_or_scalar<E1,E2>, binop<E1,E2,Mul<E1,E2> > >::T
    operator*(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Mul<E1,E2> >(e1,e2);
    }

#ifdef MKL
    array<float> operator/(float x, const array<float>& y);
    array<float> operator/(double x, const array<float>& y);
    array<double> operator/(double x, const array<double>& y);
    array<float> operator/(const array<float>& x, const array<float>& y);
    array<double> operator/(const array<double>& x, const array<double>& y);
      
    template <class E> 
    typename enable_if<is_expression<E>, array<typename E::value_type> >::T
    operator/(typename E::value_type x, const E& y) 
    {
      array<typename E::value_type> Y(y.size()); Y=y;
      return operator/(x,Y);
    }

    template <class E1, class E2>
    typename enable_if< both_are_expressions<E1,E2>, 
                        array<typename result<typename E1::value_type, typename E2::value_type>::value_type> >::T
    operator/(const E1& x, const E2& y) 
    {
      array<typename E1::value_type> X(x.size()); X=x;
      array<typename E2::value_type> Y(y.size()); Y=y;
      return operator/(X,Y);
    }

    template <class E1, class E2>
    typename enable_if< is_expression_and_scalar<E1,E2>, binop<E1,E2,Div<E1,E2> > >::T
    operator/(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Div<E1,E2> >(e1,e2);
    }

#else

    template <class E1, class E2>
    typename enable_if<is_expression_or_scalar<E1,E2>, binop<E1,E2,Div<E1,E2> > >::T
    operator/(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Div<E1,E2> >(e1,e2);
    }

#endif

    template <class E1, class E2>
    typename enable_if<is_expression_or_scalar<E1,E2>, binop<E1,E2,Mod<E1,E2> > >::T
    operator%(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Mod<E1,E2> >(e1,e2);
    }
      
    namespace both_are_expressions_ns
    {
      /* concatenation */
      template <class E1, class E2>
      typename enable_if< 
        both_are_expressions<E1,E2>, 
        array<
          typename result<
            typename E1::value_type,
            typename E2::value_type
            >::value_type,
          typename E1::Allocator
          >
        >::T
      operator<<(const E1& e1, const E2& e2) 
      {
        array<
          typename result<
            typename E1::value_type,
            typename E2::value_type
            >::value_type,
          typename E1::Allocator
          >  ret(e1.size()+e2.size(),e1.allocator());
        for (size_t i=0; i<e1.size(); i++)
          ret[i]=e1[i];
        for (size_t i=0; i<e2.size(); i++)
          ret[i+e1.size()]=e2[i];
        return ret;
      }
    } 
     
    namespace expression_scalar_ns
    {
      /* concatenation */
      template <class E1, class E2>
      typename enable_if< 
        is_expression<E1>, 
        typename enable_if< is_scalar<E2>,
                            array<
                              typename result<typename E1::value_type,E2>::value_type,
                              typename E1::Allocator>
                            >::T
        >::T
      operator<<(const E1& e1, const E2& e2) 
      {
        array<
          typename result<typename E1::value_type,E2>::value_type,
          typename E1::Allocator> ret(e1.size()+1,e1.allocator());
        for (size_t i=0; i<e1.size(); i++)
          ret[i]=e1[i];
        ret[e1.size()]=e2;
        return ret;
      }
    }

    namespace scalar_expression_ns
    {
      /* concatenation */
      template <class E1, class E2>
      typename enable_if< 
        is_scalar<E1>, 
        typename enable_if< is_expression<E2>, 
                            array<
                              typename result<E1,typename E2::value_type>::value_type,
                              typename E2::Allocator
                              >
                            >::T
        >::T
      operator<<(const E1& e1, const E2& e2) 
      {
        array<
          typename result<E1,typename E2::value_type>::value_type,
          typename E2::Allocator
          >
          ret(e2.size()+1,e2.allocator());
        ret[0]=e1;
        for (size_t i=0; i<e2.size(); i++)
          ret[i+1]=e2[i];
        return ret;
      }
    }      
  
    using both_are_expressions_ns::operator<<;
    using expression_scalar_ns::operator<<;
    using scalar_expression_ns::operator<<;
      
    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Eq<E1,E2> > >::T
    operator==(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Eq<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Neq<E1,E2> > >::T
    operator!=(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Neq<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Gt<E1,E2> > >::T
    operator>(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Gt<E1,E2> >(e1,e2);
    }
      
    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Gte<E1,E2> > >::T
    operator>=(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Gte<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Lt<E1,E2> > >::T
    operator<(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Lt<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Lte<E1,E2> > >::T
    operator<=(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Lte<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,And<E1,E2> > >::T
    operator&&(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,And<E1,E2> >(e1,e2);
    }

    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Or<E1,E2> > >::T
    operator||(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Or<E1,E2> >(e1,e2);
    }

    /**
       Convenience function for testing equality between two expressions
       Equivalent to all(e1==e2)
    */
    template <class E1, class E2>
    bool eq(const E1& e1, const E2& e2) 
    {
      if(
         array_ns::len(e1)!=1 && array_ns::len(e2)!=1 && 
         array_ns::len(e1)!=array_ns::len(e2)
         )
        return false;
      for (size_t i=0; i<std::max(array_ns::len(e1), array_ns::len(e2)); i++)
        if(array_ns::at(e1,i)!=array_ns::at(e2,i))
          return false;
      return true;
    }

    /* Functions */
#ifndef MKL

    /* support class for \c pow function */ 
    template <class E1, class E2>
    struct Pow
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {
        return std::pow(static_cast<value_type>(x),static_cast<value_type>(y));
      }
    };

    /** \c pow(x,y) elementwise \f$x^y\f$ */
    template <class E1, class E2>
    binop<E1,E2,Pow<E1,E2> > pow(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Pow<E1,E2> >(e1,e2);
    }
  
    /* support class for \c exp function */ 
    template <class T>
    struct Exp
    {
      typedef T value_type;
      T operator()(T x) const {return std::exp(x);}
    };
   
    /** elementwise exponential */
    template <class E>
    unop<E,Exp<typename E::value_type> > exp(const E& e) 
    {return  unop<E,Exp<typename E::value_type> >(e);}

    /* support class for \c log function */ 
    template <class T>
    struct Log
    {
      typedef T value_type;
      T operator()(T x) const {return std::log(x);}
    };
   
    /** elementwise natural logarithm */
    template <class E>
    unop<E,Log<typename E::value_type> > log(const E& e) 
    {return  unop<E,Log<typename E::value_type> >(e);}
      
    /* support class for \c log10 function */ 
    template <class T>
    struct Log10
    {
      typedef T value_type;
      T operator()(T x) const {return std::log10(x);}
    };
   
    /** elementwise common logarithm */
    template <class E>
    unop<E,Log10<typename E::value_type> > log10(const E& e) 
    {return  unop<E,Log10<typename E::value_type> >(e);}

    /* support class for \c sin function */ 
    template <class T>
    struct Sin
    {
      typedef T value_type;
      T operator()(T x) const {return std::sin(x);}
    };

    /** elementwise sine */   
    template <class E>
    unop<E,Sin<typename E::value_type> > sin(const E& e) 
    {return  unop<E,Sin<typename E::value_type> >(e);}

    /* support class for \c cos function */ 
    template <class T>
    struct Cos
    {
      typedef T value_type;
      T operator()(T x) const {return std::cos(x);}
    };
   
    /** elementwise cosine */   
    template <class E>
    unop<E,Cos<typename E::value_type> > cos(const E& e) 
    {return  unop<E,Cos<typename E::value_type> >(e);}

    /* support class for \c tan function */ 
    template <class T>
    struct Tan
    {
      typedef T value_type;
      T operator()(T x) const {return std::tan(x);}
    };
     
    /** elementwise tangent */   
    template <class E>
    unop<E,Tan<typename E::value_type> > tan(const E& e) 
    {return  unop<E,Tan<typename E::value_type> >(e);}
  
    /* support class for \c asin function */ 
    template <class T>
    struct Asin
    {
      typedef T value_type;
      T operator()(T x) const {return std::asin(x);}
    };
   
    /** elementwise arcsine */   
    template <class E>
    unop<E,Asin<typename E::value_type> > asin(const E& e) 
    {return  unop<E,Asin<typename E::value_type> >(e);}

    /* support class for \c acos function */ 
    template <class T>
    struct Acos
    {
      typedef T value_type;
      T operator()(T x) const {return std::acos(x);}
    };
   
    /** elementwise arccosine */   
    template <class E>
    unop<E,Acos<typename E::value_type> > acos(const E& e) 
    {return  unop<E,Acos<typename E::value_type> >(e);}

    /* support class for \c atan function */ 
    template <class T>
    struct Atan
    {
      typedef T value_type;
      T operator()(T x) const {return std::atan(x);}
    };
      
    /** elementwise arctangent */   
    template <class E>
    unop<E,Atan<typename E::value_type> > atan(const E& e) 
    {return  unop<E,Atan<typename E::value_type> >(e);}
  
    /* support class for \c atan2 function */ 
    template <class E1, class E2>
    struct Atan2
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return std::atan2(x,y);}
    };

    /** elementwise \c atan2(x,y) \f$=\tan^{-1}(x/y)\in[-\pi/2,\pi/2]\f$ */   
    template <class E1, class E2>
    binop<E1,E2,Atan2<E1,E2> > atan2(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Atan2<E1,E2> >(e1,e2);
    }
  
    /* support class for \c sinh function */ 
    template <class T>
    struct Sinh
    {
      typedef T value_type;
      T operator()(T x) const {return std::sinh(x);}
    };
   
    /** elementwise hyperbolic sine */   
    template <class E>
    unop<E,Sinh<typename E::value_type> > sinh(const E& e) 
    {return  unop<E,Sinh<typename E::value_type> >(e);}
      
    /* support class for \c cosh function */ 
    template <class T>
    struct Cosh
    {
      typedef T value_type;
      T operator()(T x) const {return std::cosh(x);}
    };
   
    /** elementwise hyperbolic cosine */   
    template <class E>
    unop<E,Cosh<typename E::value_type> > cosh(const E& e) 
    {return  unop<E,Cosh<typename E::value_type> >(e);}
      
    /* support class for \c tanh function */ 
    template <class T>
    struct Tanh
    {
      typedef T value_type;
      T operator()(T x) const {return std::tanh(x);}
    };
      
    /** elementwise hyperbolic tangent */   
    template <class E>
    unop<E,Tanh<typename E::value_type> > tanh(const E& e) 
    {return  unop<E,Tanh<typename E::value_type> >(e);}
      
    /* support class for \c sqrt function */ 
    template <class T>
    struct Sqrt
    {
      typedef T value_type;
      T operator()(T x) const {return std::sqrt(x);}
    };
   
    /** elementwise square root */   
    template <class E>
    unop<E,Sqrt<typename E::value_type> > sqrt(const E& e) 
    {return  unop<E,Sqrt<typename E::value_type> >(e);}
#endif  // ! MKL

    /* support class for \c abs function */ 
    template <class T>
    struct Abs
    {
      typedef T value_type;
      T operator()(T x) const {return std::abs(x);}
    };
   
    /* support class for \c SGN function */ 
    template <class T>
    struct Sgn
    {
      typedef T value_type;
      T operator()(T x) const {return x>=0? 1: -1;}
    };
   
    /** elementwise absolute value \c fabs(x)\f$=|x|\f$ */     
    template <class E>
    unop<E,Abs<typename E::value_type> > fabs(const E& e) 
    {return  unop<E,Abs<typename E::value_type> >(e);}

    /** elementwise absolute value \c abs(x)\f$=|x|\f$ */     
    template <class E>
    unop<E,Abs<typename E::value_type> > abs(const E& e) 
    {return  unop<E,Abs<typename E::value_type> >(e);}

    /** elementwise signum \c sign(x)\f$=\left\{ 
        \begin{array}{ll}
        1& x\geq0\\
        -1&x<0\\
        \end{array}\right.\f$ */     
    template <class E>
    unop<E,Sgn<typename E::value_type> > sign(const E& e) 
    {return  unop<E,Sgn<typename E::value_type> >(e);}

    /* support class for \c ceil function */ 
    template <class T>
    struct Ceil
    {
      typedef T value_type;
      T operator()(T x) const {return std::ceil(x);}
    };
   
    /** elementwise \c ceil(x)\f$=\lceil x\rceil\f$ */     
    template <class E>
    unop<E,Ceil<typename E::value_type> > ceil(const E& e) 
    {return  unop<E,Ceil<typename E::value_type> >(e);}

    /* support class for \c floor function */ 
    template <class T>
    struct Floor
    {
      typedef T value_type;
      T operator()(T x) const {return std::floor(x);}
    };
   
    /** elementwise \c floor(x)\f$=\lfloor x\rfloor\f$ */     
    template <class E>
    unop<E,Floor<typename E::value_type> > floor(const E& e) 
    {return  unop<E,Floor<typename E::value_type> >(e);}

    /* support class for \c ldexp function */ 
    template <class E1, class E2>
    struct Ldexp
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return std::ldexp(x,y);}
    };

    /** elementwise \c ldexp(x,n)\f$=x\times2^n\f$ */     
    template <class E1, class E2>
    binop<E1,E2,Ldexp<E1,E2> > ldexp(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Ldexp<E1,E2> >(e1,e2);
    }

    /* support class for \c fmod function */ 
    template <class E1, class E2>
    struct Fmod
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {return std::fmod(x,y);}
    };

    /** elementwise \c fmod(x,y) = floating point remainder of x/y */     
    template <class E1, class E2>
    binop<E1,E2,Fmod<E1,E2> > fmod(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Fmod<E1,E2> >(e1,e2);
    }

    /* support class for \c max function */ 
    template <class E1, class E2>
    struct Max
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {
        return std::max(static_cast<value_type>(x),static_cast<value_type>(y));
      }
    };

    /** elementwise maximum of x and y */     
    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Max<E1,E2> > >::T
    max(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Max<E1,E2> >(e1,e2);
    }

    /* support class for \c min function */ 
    template <class E1, class E2>
    struct Min
    {
      typedef typename traits<E1>::value_type E1v;
      typedef typename traits<E2>::value_type E2v;
      typedef typename result<E1v,E2v>::value_type value_type;
      value_type operator()(E1v x, E2v y) const {
        return std::min(static_cast<value_type>(x),static_cast<value_type>(y));
      }
    };

    /** elementwise minimum of x and y */     
    template <class E1, class E2>
    typename enable_if< one_is_expression<E1,E2>, binop<E1,E2,Min<E1,E2> > >::T
    min(const E1& e1, const E2& e2) 
    {
      return binop<E1,E2,Min<E1,E2> >(e1,e2);
    }



    /* support class for unassignable vector index expressions */
    template <class E, class I>
    class RVindex 
    {
      const E& expr;
      const I& idx;
      void operator=(const RVindex&);
    public:
      typedef typename E::value_type value_type;
      size_t size() const {return idx.size();}
      using Allocator=typename E::Allocator;
      const Allocator& allocator() const {return expr.allocator();}
      value_type operator[](size_t i) const {return expr[idx[i]];}
      RVindex(const E& e,const I& i): expr(e), idx(i) {}
    };
  
    /* support class for assignable vector index expressions (lvalues) */
    template <class E, class I>
    class LVindex  
    {
      E& expr;
      const I& idx;
    public:
      typedef typename E::value_type value_type;
      size_t size() const {return idx.size();}
      using Allocator=typename E::Allocator;
      const Allocator& allocator() const {return expr.allocator();}
      value_type& operator[](size_t i) {return expr[idx[i]];}
      value_type operator[](size_t i) const {return expr[idx[i]];}
      LVindex(E& e,const I& i): expr(e), idx(i) {}
      template <class E1> typename
      enable_if <is_expression<E1>, RVindex<E,I> >::T
      operator=(const E1& x) {
        conformance_check(idx,x);
        for (size_t i=0; i<size(); i++) expr[idx[i]]=x[i];
        return RVindex<E,I>(expr,idx);
      }
      template <class E1> typename
      enable_if <is_expression<E1>, LVindex& >::T
      operator+=(const E1& x) {
        conformance_check(idx,x);
        for (size_t i=0; i<size(); i++) 
          expr[idx[i]]+=x[i];
        //      return RVindex<E,I>(expr,idx);
        return *this;
      }
      template <class E1> typename
      enable_if <is_expression<E1>, RVindex<E,I> >::T
      operator*=(const E1& x) {
        conformance_check(idx,x);
        for (size_t i=0; i<size(); i++) expr[idx[i]]*=x[i];
        return RVindex<E,I>(expr,idx);
      }
      template <class E1> typename
      enable_if <is_expression<E1>, RVindex<E,I> >::T
      operator&=(const E1& x) {
        conformance_check(idx,x);
        for (size_t i=0; i<size(); i++) expr[idx[i]]&=x[i];
        return RVindex<E,I>(expr,idx);
      }
      template <class E1> typename
      enable_if <is_expression<E1>, RVindex<E,I> >::T
      operator|=(const E1& x) {
        conformance_check(idx,x);
        for (size_t i=0; i<size(); i++) expr[idx[i]]|=x[i];
        return RVindex<E,I>(expr,idx);
      }
      value_type operator=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]=x;
        return x;
      }
      value_type operator+=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]+=x;
        return x;
      }
      value_type operator-=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]-=x;
        return x;
      }
      value_type operator*=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]*=x;
        return x;
      }
      value_type operator/=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]/=x;
        return x;
      }
      value_type operator%=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]%=x;
        return x;
      }
      value_type operator&=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]&=x;
        return x;
      }
      value_type operator|=(value_type x) {
        for (size_t i=0; i<size(); i++) expr[idx[i]]|=x;
        return x;
      }
    };
  
    // Vector indices are also expressions
    template <class E, class I> struct is_expression<RVindex<E,I> >
    {static const bool value=true;};
    // Vector indices are also expressions
    template <class E, class I> struct is_expression<LVindex<E,I> >
    {static const bool value=true;};

    /* unary operator adaptor */
    template <class E, class Op>
    class unop
    {
    public:
      const E& e;
      Op op;

      unop(const E& expr): e(expr) {}
      /// type of result
      typedef typename E::value_type value_type;
      size_t size() const {return e.size();}

      value_type operator[](size_t i) const {return op(e[i]);}

      /// vector indexing
      template <class I> typename 
      enable_if< is_expression<I>, RVindex<unop,I> >::T
      operator[](const I& i) const {return RVindex<unop,I>(*this,i);}

      using Allocator=typename E::Allocator;
      const Allocator& allocator() const {return e.allocator();}
    };

    /* binary operator adaptor */
    template <class E1, class E2, class Op>
    class binop
    {
    public:
      const E1& e1;
      const E2& e2;
      Op op;

      binop(const E1& ex1, const E2& ex2): e1(ex1), e2(ex2) {conformance_check(e1,e2);}
      /// type of result
      typedef typename Op::value_type value_type;
      size_t size() const 
      {return (array_ns::len(e1)==1)? array_ns::len(e2): array_ns::len(e1);}
      typename Op::value_type operator[](size_t i) const {
        /* use at function here, as E1 or E2 may be scalar */
        return op(array_ns::at(e1,i),array_ns::at(e2,i));
      }

      // vector indexing
      template <class I> typename 
      enable_if< is_expression<I>, RVindex<binop,I> >::T
      operator[](const I& i) const {return RVindex<binop,I>(*this,i);}

      using Allocator=typename classdesc::conditional<
        is_expression<E1>::value,
        typename Allocator<E1>::type,
        typename Allocator<E2>::type
        >::T;
      template <class U>
      typename enable_if<is_expression<U>, const Allocator&>::T
      allocator_(const E1& e1, const E2& e2) const {return e1.allocator();}
      template <class U>
      typename enable_if<classdesc::Not<is_expression<U>>, const Allocator&>::T
      allocator_(const E1& e1, const E2& e2) const {return e2.allocator();}
      const Allocator& allocator() const {return allocator_<E1>(e1,e2);}
    };

    /*
      specialisation that converts divisions into multiplications by a reciprocal 
    */
    template <class E1>
    class binop<E1,double,Div<E1,double> >
    {
    public:
      const E1& e1;
      double e2;

      binop(const E1& ex1, double ex2): e1(ex1), e2(1/ex2) {conformance_check(e1,e2);}
      typedef typename result<typename E1::value_type,double>::value_type value_type;
      size_t size() const 
      {return (array_ns::len(e1)==1)? array_ns::len(e2): array_ns::len(e1);}
      /* use at function here, as E1 or E2 may be scalar */
      value_type operator[](size_t i) const {return array_ns::at(e1,i)*e2;}

      /// vector indexing
      template <class I> typename 
      enable_if< is_expression<I>, RVindex<binop,I> >::T
      operator[](const I& i) const {return RVindex<binop,I>(*this,i);}
    };

    /* 
       specialisation that converts divisions into multiplications by a reciprocal 
    */
    template <class E1>
    class binop<E1,float,Div<E1,float> >
    {
    public:
      const E1& e1;
      float e2;

      binop(const E1& ex1, float ex2): e1(ex1), e2(1/ex2) {conformance_check(e1,e2);}
      typedef typename result<typename E1::value_type,float>::value_type value_type;
      size_t size() const 
      {return (array_ns::len(e1)==1)? array_ns::len(e2): array_ns::len(e1);}
      /* use at function here, as E1 or E2 may be scalar */
      value_type operator[](size_t i) const {return array_ns::at(e1,i)*e2;}

      // vector indexing
      template <class I> typename 
      enable_if< is_expression<I>, RVindex<binop,I> >::T
      operator[](const I& i) const {return RVindex<binop,I>(*this,i);}
    };

    /* internal support */
    template <class T>
    void asg(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]=x;});
    }

    template <class T>
    void asg_plus(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]+=x;});
    }

    /* internal support */
    template <class T>
    void asg_minus(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]-=x;});
    }
    /* internal support */
    template <class T>
    void asg_mul(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]*=x;});
    }

    /* internal support */
    template <class T>
    void asg_div(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]/=x;});
    }


    /* internal support */
    template <class T> typename
    enable_if< is_integer<T>, void >::T
    asg_mod(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]%=x;});
    }


    /* internal support */
    template <class T> typename
    enable_if< is_integer<T>, void >::T
    asg_and(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]&=x;});
    }


    /* internal support */
    template <class T> typename
    enable_if< is_integer<T>, void >::T
    asg_or(T* dt, size_t sz, T x) {
      map(sz,[&](size_t i){dt[i]|=x;});
    }


    /* internal support */
    template <class T, class U>
    void asg_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_plus_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]+=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_minus_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]-=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_mul_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]*=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_div_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]/=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_mod_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]%=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_and_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]&=x[i];});
    }

    /* internal support */
    template <class T, class U>
    void asg_or_v(T * dt, size_t sz, const U& x) {
      map(sz,[&](size_t i){dt[i]|=x[i];});
    }

    /* layout of array data, including fixed fields */
    template <class T>
    struct array_data
    {
      T *allocated_pointer; //actual pointer allocated (for alignment purposes)
      std::size_t sz; //array size
      std::size_t allocation; // actual allocation
      unsigned cnt;   //reference count
      static const unsigned debug_display=10; //allows debuggers to see first debug_display elem
      T dt[debug_display];       //data 
    };

    /**
       array is the base template class for array types to follow
    */

    template <class T, class A=std::allocator<T>>
    class array
    {
      array_data<T> *dt;
      A m_allocator;

      
      friend class WhereContext;

      ///allocate \a n variables of type \a T 
      array_data<T> *alloc(std::size_t n)
      {
        T *p; 
        array_data<T> *r;
        //p = (char*)std::malloc((n-array_data<T>::debug_display) * sizeof(T) + sizeof(array_data<T>) + 16);
        // over allocate to allow for alignment and metadata
        auto allocation=n + (sizeof(array_data<T>) + 16)/sizeof(T)+1-array_data<T>::debug_display;
        p = m_allocator.allocate(allocation);
      
        if (!p) return nullptr; // SYCL allocator returns nullptr if not initialised
#ifdef __ICC
        // we need to align data onto 16 byte boundaries
        size_t d = (size_t)(reinterpret_cast<array_data<T>*>(p)->dt);
        size_t offs = (16 - (d&15)) & 15;
        r=reinterpret_cast<array_data<T>*>(p+offs);
#else
        r=reinterpret_cast<array_data<T>*>(p);
#endif
        r->allocated_pointer=p;
        r->allocation=allocation;
        r->sz=n;
        r->cnt=1;
        return r;
      }

      ///free memory pointed to by \a p 
      void free(array_data<T> *p)
      {
        assert(p);
        //std::free(p->allocated_pointer);
        m_allocator.deallocate(p->allocated_pointer,p->allocation);
      }

      void set_size(size_t s) {dt = alloc(s);}

#if defined(SYCL_LANGUAGE_VERSION)
      using AtomicUnsignedRef=sycl::atomic_ref
        <unsigned,sycl::memory_order::relaxed,sycl::memory_scope::device>;
#else
      using AtomicUnsignedRef=unsigned&;
#endif
      
      AtomicUnsignedRef ref() // access reference counter
      {
        assert(dt);
        return AtomicUnsignedRef(dt->cnt);
      }

      void release()
      {
        if (dt)
          {
//#if defined(SYCL_LANGUAGE_VERSION) && !defined(__SYCL_DEVICE_ONLY__)
//            // dt pointer may be allocated on device, or in device
//            // memory, and we may be running on the host, in which
//            // case just leak the memory, otherwise we'll have a crash
//            // TODO - call release on device in a single_task for the
//            // first situation
//            // TODO in second situation, update ref
//            // count in a single_task, and pass back value of
//            // allocated pointer for deallocation on host
//            if (is_same<A,typename CellBase::CellAllocator<T>>::value ||
//                sycl::get_pointer_type(dt,syclQ().get_context())==sycl::usm::alloc::device) return;
//#endif
            if (ref()==1)
              {
                free(dt);
              }
            else
              ref()--;
          }
      }

    protected:

      void copy() //any nonconst method needs to call this
      {           // to implement copy-on-write semantics
        if (dt && ref()>1)
          {
            array_data<T>* oldData=dt;
            bool freeMem = ref()-- == 0;
            dt=alloc(size()); 
            memcpy(dt->dt,oldData->dt,size()*sizeof(T));
            if (freeMem) free(oldData);
          }
      }

    public:
      typedef T value_type;
      typedef size_t size_type; 
      using Allocator=A;

      array(const Allocator& alloc={}): m_allocator(alloc) {set_size(0);}
      explicit array(size_t s, const Allocator& alloc=Allocator()): m_allocator(alloc)
      {
        set_size(s);
      }

      array(size_t s, T val, const Allocator& alloc={}): m_allocator(alloc)
      {
        set_size(s);
        array_ns::asg(data(),size(),val);
      }

      array(const array& x): m_allocator(x.m_allocator) 
      {
        dt=x.dt;
        if (dt) ref()++;
      }

      template <class expr>
      array(const expr& e, const Allocator& alloc={},
            typename enable_if< is_expression<expr>, void*>::T dummy=0): m_allocator(alloc)
      {
        set_size(e.size());
        operator=(e);
      }

      ~array() {release();}

      const Allocator& allocator() const {return m_allocator;}
      const Allocator& allocator(const Allocator& alloc) {
        if (alloc==m_allocator) return m_allocator;
        array tmp(size(),alloc);
        asg_v(tmp.data(),size(),data());
        swap(tmp);
        return m_allocator;
      }
      
      /// resize array to \a s elements
      void resize(size_t s) {
        if (!dt || s>dt->sz || ref()>1)
          {
            release();
            dt = alloc(s);
          } 
        if (dt) dt->sz=s; // in case s is smaller
      } 

      void clear() {resize(0);}
      
      /// resize array to \a s elements, and initialise to \a val
      template <class V>
      void resize(size_t s, const V& val) {resize(s); operator=(val);}

      void swap(array& x) {
        std::swap(dt, x.dt);
        std::swap(m_allocator,x.m_allocator);
      }

      T& operator[](size_t i) {assert(i<size()); copy(); return data()[i];}
      T operator[](size_t i) const {assert(i<size()); return data()[i];}

      /// vector indexing
      template <class I> typename 
      enable_if< is_expression<I>, RVindex<array,I> >::T
      operator[](const I& i) const {return RVindex<array,I>(*this,i);}
      template <class I> typename
      enable_if< is_expression<I>, LVindex<array,I> >::T
      operator[](const I& i) {copy(); return LVindex<array,I>(*this,i);}

      array& operator=(const array& x) {
        if (x.dt==dt) return *this;
        if (m_allocator==x.m_allocator) { // shared data optimisation
          release();
          dt=x.dt;
          if (dt) ref()++;
          return *this;
        } 
        array tmp(x.size(),m_allocator);
        array_ns::asg_v(tmp.data(),tmp.size(),x);
        swap(tmp);
        return *this;
      }

      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T 
      operator=(const expr& x) {
        if ((void*)(&x)==(void*)(this)) return *this;
        // since expression x may contain a reference to this, assign to a temporary
#ifdef __SYCL_DEVICE_ONLY__
        GroupLocal<array> tmp(x.size(),m_allocator);
        array_ns::asg_v(tmp.ref().data(),x.size(),x);
        groupBarrier();
        if (syclGroup().leader()) swap(tmp.ref());
#else
        array tmp(x.size(),m_allocator);
        array_ns::asg_v(tmp.data(),tmp.size(),x);
        swap(tmp);
#endif
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T 
      operator+=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_plus_v(data(),size(),x);
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T  
      operator-=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_minus_v(data(),size(),x);
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T
      operator*=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_mul_v(data(),size(),x);
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T
      operator/=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_div_v(data(),size(),x);
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T
      operator%=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_mod_v(data(),size(),x);
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T
      operator&=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_and_v(data(),size(),x);
        return *this;
      }
      template <class expr> typename
      enable_if<is_expression<expr>, array&>::T
      operator|=(const expr& x) {
        assert(size()==x.size());
        copy();
        array_ns::asg_or_v(data(),size(),x);
        return *this;
      }

      /// concatenation 
      template <class E> 
      typename enable_if<is_expression<E>,array&>::T
      operator<<=(const E& x) {
        array orig(*this);
        resize(orig.size()+x.size());
        asg_v(data(),orig.size(),orig);
        asg_v(data()+orig.size(),x.size(),x);
        return *this;
      }

      template <class S> 
      typename enable_if<is_scalar<S>,array&>::T
      operator<<=(S x) {
        array orig(*this);
        resize(orig.size()+1);
        asg_v(data(),orig.size(),orig);
        data()[orig.size()]=x;
        return *this;
      }

      /// broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator=(scalar x) {copy(); asg(data(),size(),T(x)); return *this;}

      /// summing broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator+=(scalar x) {copy(); asg_plus(data(),size(),T(x)); return *this;}

      /// subtracting broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator-=(scalar x) {copy(); asg_minus(data(),size(),T(x)); return *this;}

      /// multiplying broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator*=(scalar x) {copy(); asg_mul(data(),size(),T(x)); return *this;}

      /// dividing broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator/=(scalar x)          {copy(); asg_div(data(),size(),T(x)); return *this;}

      /// remaindering broadcast
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator%=(scalar x) {copy(); asg_mod(data(),size(),T(x)); return *this;}

      /// bitwise and broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator&=(scalar x) {copy(); asg_and(data(),size(),T(x)); return *this;}

      /// bitwise or broadcast 
      template <class scalar> typename
      enable_if<is_scalar<scalar>, array&>::T
      operator|=(scalar x) {copy(); asg_or(data(),size(),T(x)); return *this;}

//      binop<array,double,Mul<array,double> > >::T
//      operator/(double x) const {return array_ns::operator*(*this,1/x);}

      /// number of elements
      size_t size() const {return dt? dt->sz: 0;}
      /// obtain raw pointer to data
      T* data() {copy(); return dt? dt->dt: 0;}
      /// obtain raw pointer to data
      const T* data() const {return dt? dt->dt: 0;}

      /// returns a writeable pointer to data without copy-on-write semantics
      /// dangerous, but needed to run array expressions on GPU
      //T* dataNoCow() {return dt? dt->dt: 0;}
      
      typedef T *iterator;
      typedef const T *const_iterator;
    
      iterator begin(void) {copy(); return iterator(data()); }
      iterator end(void) {copy();  return iterator(data()+size()); }

      const_iterator begin(void) const { return const_iterator(data()); }
      const_iterator end(void) const { return const_iterator(data() + size()); }
    };

#ifdef MKL
    /** specialisations to call out to the MKL vector transcendental library */
    inline array<float> operator/(float x, const array<float>& y)
    {
      array<float> r(y.size());
      vsInv(static_cast<int>(y.size()),y.data(),r.data());
      if (x!=1) r*=x;
      return r;
    }

    inline array<float> operator/(double x, const array<float>& y)
    {return operator/(static_cast<float>(x),y);}

    inline array<double> operator/(double x, const array<double>& y)
    {
      array<double> r(y.size());
      vdInv(static_cast<int>(y.size()),y.data(),r.data());
      if (x!=1) r*=x;
      return r;
    }

    inline array<float> operator/(const array<float>& x, const array<float>& y)
    {
      conformance_check(x,y);
      array<float> r(x.size());
      vsDiv(static_cast<int>(x.size()),x.data(),y.data(),r.data());
      return r;
    }

    inline array<double> operator/(const array<double>& x, const array<double>& y)
    {
      conformance_check(x,y);
      array<double> r(x.size());
      vdDiv(static_cast<int>(x.size()),x.data(),y.data(),r.data());
      return r;
    }

    /* MKL can perform inplace operations */
    inline void asg_div_v(float *dt, size_t sz, array<float>& x)
    {vsDiv(static_cast<int>(sz),dt,x.data(),dt);}

    inline void asg_div_v(double *dt, size_t sz, array<double>& x)
    {vdDiv(static_cast<int>(sz),dt,x.data(),dt);}

    inline array<float> sqrt(const array<float>& x)
    {
      array<float> r(x.size());
      vsSqrt(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }
  
    inline array<double> sqrt(const array<double>& x)
    {
      array<double> r(x.size());
      vdSqrt(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }
  
    inline array<float> pow(const array<float>& x, float y)
    {
      array<float> r(x.size());
      vsPowx(static_cast<int>(x.size()),x.data(),y,r.data());
      return r;
    }

    inline array<float> pow(const array<float>& x, double y)
    {return pow(x,static_cast<float>(y));}

    inline array<double> pow(const array<double>& x, double y)
    {
      array<double> r(x.size());
      vdPowx(static_cast<int>(x.size()),x.data(),y,r.data());
      return r;
    }

    inline array<float> pow(const array<float>& x, const array<float>& y)
    {
      array<float> r(x.size());
      conformance_check(x,y);
      vsPow(static_cast<int>(x.size()),x.data(),y.data(),r.data());
      return r;
    }

    inline array<double> pow(const array<double>& x, const array<double>& y)
    {
      array<double> r(x.size());
      conformance_check(x,y);
      vdPow(static_cast<int>(x.size()),x.data(),y.data(),r.data());
      return r;
    }

    inline array<float> exp(const array<float>& x)
    {
      array<float> r(x.size());
      vsExp(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> exp(const array<double>& x)
    {
      array<double> r(x.size());
      vdExp(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> log(const array<float>& x)
    {
      array<float> r(x.size());
      vsLn(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> log(const array<double>& x)
    {
      array<double> r(x.size());
      vdLn(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> log10(const array<float>& x)
    {
      array<float> r(x.size());
      vsLog10(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> log10(const array<double>& x)
    {
      array<double> r(x.size());
      vdLog10(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> cos(const array<float>& x)
    {
      array<float> r(x.size());
      vsCos(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> cos(const array<double>& x)
    {
      array<double> r(x.size());
      vdCos(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> sin(const array<float>& x)
    {
      array<float> r(x.size());
      vsSin(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> sin(const array<double>& x)
    {
      array<double> r(x.size());
      vdSin(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> tan(const array<float>& x)
    {
      array<float> r(x.size());
      vsTan(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> tan(const array<double>& x)
    {
      array<double> r(x.size());
      vdTan(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> asin(const array<float>& x)
    {
      array<float> r(x.size());
      vsAsin(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> asin(const array<double>& x)
    {
      array<double> r(x.size());
      vdAsin(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> acos(const array<float>& x)
    {
      array<float> r(x.size());
      vsAcos(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> acos(const array<double>& x)
    {
      array<double> r(x.size());
      vdAcos(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> atan(const array<float>& x)
    {
      array<float> r(x.size());
      vsAtan(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> atan(const array<double>& x)
    {
      array<double> r(x.size());
      vdAtan(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> atan2(const array<float>& x, const array<float>& y)
    {
      array<float> r(x.size());
      conformance_check(x,y);
      vsAtan2(static_cast<int>(x.size()),x.data(),y.data(),r.data());
      return r;
    }

    inline array<double> atan2(const array<double>& x, const array<double>& y)
    {
      array<double> r(x.size());
      conformance_check(x,y);
      vdAtan2(static_cast<int>(x.size()),x.data(),y.data(),r.data());
      return r;
    }

    inline array<float> cosh(const array<float>& x)
    {
      array<float> r(x.size());
      vsCosh(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> cosh(const array<double>& x)
    {
      array<double> r(x.size());
      vdCosh(static_cast<int>(x.size()),x.data(),r.data());
 r;
    }

    inline array<float> sinh(const array<float>& x)
    {
      array<float> r(x.size());
      vsSinh(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> sinh(const array<double>& x)
    {
      array<double> r(x.size());
      vdSinh(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<float> tanh(const array<float>& x)
    {
      array<float> r(x.size());
      vsTanh(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }

    inline array<double> tanh(const array<double>& x)
    {
      array<double> r(x.size());
      vdTanh(static_cast<int>(x.size()),x.data(),r.data());
      return r;
    }
    
    /* apply the above functions to expressions */

    template <class E> typename enable_if<is_expression<E>, void >::T
    asg_div_v(typename E::value_type *dt, size_t sz, const E& y) 
    {
      array<typename E::value_type> Y(y.size()); Y=y;
      asg_div_v(dt,sz,Y);
    }

    template <class E> typename enable_if<is_expression<E>, array<typename E::value_type> >::T
    pow(const E& x, typename E::value_type y) 
    {
      array<typename E::value_type> X(x);  
      return pow(X,y);
    }

    template <class E1, class E2> typename
    enable_if<one_is_expression<E1,E2>, 
              array<typename result< typename E1::value_type, typename E2::value_type>::value_type> >::T
    pow(const E1& x, const E2& y) 
    {
      array<typename result<typename E1::value_type,typename E2::value_type>::value_type> 
        X(x.size()), Y(y.size()); X=x; Y=y; 
      return pow(X,Y);
    }

    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    sqrt(const E& x) {array<typename E::value_type> X(x.size()); X=x; return sqrt(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    exp(const E& x) {array<typename E::value_type> X(x.size()); X=x; return exp(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    log(const E& x) {array<typename E::value_type> X(x.size()); X=x; return log(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    log10(const E& x) {array<typename E::value_type> X(x.size()); X=x; return log10(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    cos(const E& x) {array<typename E::value_type> X(x.size()); X=x; return cos(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    sin(const E& x) {array<typename E::value_type> X(x.size()); X=x; return sin(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    tan(const E& x) {array<typename E::value_type> X(x.size()); X=x; return tan(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    acos(const E& x) {array<typename E::value_type> X(x.size()); X=x; return acos(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    asin(const E& x) {array<typename E::value_type> X(x.size()); X=x; return asin(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    atan(const E& x) {array<typename E::value_type> X(x.size()); X=x; return atan(X);}

    template <class E1, class E2> typename
    enable_if<one_is_expression<E1,E2>, array<typename result<E1,E2>::value_type> >::T
    atan2(const E1& x, const E2& y) 
    {
      array<typename result<E1,E2>::value_type> X(x.size()), Y(y.size()); X=x; Y=y;
      return atan2(X,Y);
    }

    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    cosh(const E& x) {array<typename E::value_type> X(x.size()); X=x; return cosh(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    sinh(const E& x) {array<typename E::value_type> X(x.size()); X=x; return sinh(X);}
    template <class E> typename
    enable_if<is_expression<E>, array<typename E::value_type> >::T
    tanh(const E& x) {array<typename E::value_type> X(x.size()); X=x; return tanh(X);}

#endif // MKL

    /* pos takes a boolean array expression x, and returns an integer array giving the 
       indices where x[i] is true */

    template <class E> typename
    enable_if< is_expression<E>, array<size_t> >::T
    pos(const E& x, dummy<0> d=0)
    {
      array<bool> cond=x;
      size_t ntrue=0;
      for (size_t i=0; i<cond.size(); i++) ntrue+=cond[i]; 
      array<size_t> r(ntrue);
      for (size_t i=0, j=0; i<cond.size(); i++) 
        if (cond[i]) r[j++]=i;
      return r;
    }

    template <class S> typename
    enable_if< is_scalar<S>, size_t >::T
    pos(const S& x, dummy<1> d=1) {return 0;}
  
    /*
      Boolean short cuts
    */

    /*
      We don't want to evaluate the second argument if first argument is false (&&-case) or 
      true (||-case). In such a case, return a varArray<bool> sized as a fixedArray<bool> 
      (since we're expecting this to  be used in fixedArray expressions)
    */

    template <class E>
    typename enable_if< is_expression<E>, array<bool> >::T
    operator&&(bool x, const E& e)
    {
      array<bool> r;
      if (x)
        r=e;
      else
        r=array<bool>(1,false);  
      return r;
    }
  
    template <class T, class E>
    typename enable_if< is_expression<E>, array<bool> >::T
    operator&&(T *x, const E& e) 
    {return x!=NULL && e;}

    template <class E>
    typename enable_if< is_expression<E>, array<bool> >::T
    operator||(bool x, const E& e)
    {
      array<bool> r;
      if (!x)
        r=e;
      else
        r=array<bool>(1,true);  
      return r;
    }

    /* reductions */

    /// maximum value of a vector expression \f$\max_i x_i\f$
    template <class E>
    typename enable_if< is_expression<E>, typename E::value_type >::T
    max(const E& x)
    {
      typename E::value_type r;
      //strangely numeric_limits<float>::min() differs semantically from numeric_limits<int>::min()
      if (std::numeric_limits<typename E::value_type>::is_integer) 
        r=std::numeric_limits<typename E::value_type>::min();
      else
        r=-std::numeric_limits<typename E::value_type>::max();
      for (size_t i=0; i<x.size(); i++)
        r = std::max(x[i],r);
      return r;
    }

    /// minimum value of a vector expression \f$\min_i x_i\f$
    template <class E>
    typename enable_if< is_expression<E>, typename E::value_type >::T
    min(const E& x)
    {
      typename E::value_type r=std::numeric_limits<typename E::value_type>::max();
      for (size_t i=0; i<x.size(); i++)
        r = std::min(x[i],r);
      return r;
    }

    /// sum of a vector expression \f$\sum_i x_i\f$
    template <class E>
    typename enable_if< is_expression<E>, 
                        typename result<typename E::value_type, typename E::value_type>::value_type
                        >::T
    sum(const E& x)
    {
      typename result<typename E::value_type, typename E::value_type>::value_type
        r=0;
      for (size_t i=0; i<x.size(); i++)
        r += x[i];
      return r;
    }

    template <class E, class M>
    typename enable_if< is_expression<E>, 
                        typename result<typename E::value_type, typename E::value_type>::value_type
                        >::T
    sum(const E& x, const M& mask)
    {
      typename result<typename E::value_type, typename E::value_type>::value_type
        r=0;
      for (size_t i=0; i<x.size(); i++)
        if (mask[i]) r += x[i];
      return r;
    }

    /// product of a vector expression \f$\prod_i x_i\f$
    template <class E>
    typename enable_if< is_expression<E>, typename E::value_type >::T
    prod(const E& x)
    {
      typename E::value_type r=1;
      for (size_t i=0; i<x.size(); i++)
        r *= x[i];
      return r;
    }

    /// product of a vector expression \f$\prod_i x_i\f$
    template <class E, class M>
    typename enable_if< is_expression<E>, typename E::value_type >::T
    prod(const E& x, const M& mask)
    {
      typename E::value_type r=1;
      for (size_t i=0; i<x.size(); i++)
        if (mask[i]) r *= x[i];
      return r;
    }

    /// true iff any element \a x is true
    template <class E>
    bool any(const E& x)
    {
      for (size_t i=0; i<x.size(); i++)
        if(x[i])
          return true;

      return false;
    }

    /// true iff all elements of \a x are true
    template <class E>
    bool all(const E& x)
    {
      for (size_t i=0; i<x.size(); i++)
        if(!x[i])
          return false;

      return true;
    }

    /// merge two vectors: r[i] = mask[i]? x[i]: y[i]
    template <class E1, class E2, class E3> typename
    enable_if<is_expression<E1>,
              array<typename result<E2,E3>::value_type > 
              >::T
    merge(const E1& mask, const E2& x, const E3& y)
    {
      conformance_check(mask,x);
      conformance_check(mask,y);
      array<typename result<E1,E2>::value_type > ret(mask.size());
      for (size_t i=0; i<mask.size(); i++)
        ret[i]= mask[i]? at(x,i): at(y,i);
      return ret;
    }

    /// pack vector (remove elements where mask[i] is false
    template <class E1, class E2> typename
    enable_if<both_are_expressions<E1,E2>,
              array<typename E1::value_type,typename E1::Allocator>
              >::T
    pack(const E1& e, const E2& mask, long ntrue=-1)
              {
                assert(mask.size()==e.size());
                array<typename E1::value_type,typename E1::Allocator> r( (ntrue==-1)? sum(mask): ntrue, e.allocator());
                for (size_t i=0, j=0; i<mask.size(); i++)
                  if (mask[i]) r[j++]=e[i];
                return r;
              }

  /// running sum of x considered as a boolean mask
  template <class E> typename
  enable_if<is_expression<E>, array<size_t> >::T
  enumerate(const E& x)
  {
    array<size_t> r(x.size());
    for (size_t i=0, j=0; i<x.size(); i++)
      {
        r[i]=j;
        if (x[i]) j++;
      }
    return r;
  }

  template <class E> 
  std::ostream&  put(std::ostream& o, const E& x)
  {
    if (x.size())
      {
        for (size_t i=0; i<x.size()-1; i++)
          o << x[i] << " "; 
        o<<x[x.size()-1];
      }
    return o;
  }

  template <class T>
  std::ostream& operator<<(std::ostream& o, const array<T>& x)
  {return put(o,x);}

  /// ostream putter
  template <class T, class Op>
  std::ostream& operator<<(std::ostream& o, const unop<T, Op>& x)
  {return put(o,x);}

  template <class E1, class E2, class Op> 
  std::ostream& operator<<(std::ostream& o, const binop<E1,E2, Op>& x)
  {return put(o,x);}

  /// istream getter
  template <class T>
  std::istream& get(std::istream& s, T& x)
  {
    typename T::value_type v; x.resize(0);
    while (s>>v) {x<<=v;}
    return s;
  }

  template <class T>
  std::istream& operator>>(std::istream& i, array<T>& x)
  {return get(i,x);}

  /// [0...n-1]
  inline array<int> pcoord(int n)
  {
    array<int> r(n);
#ifdef __ICC
#pragma loop count(1000)
#pragma ivdep
#pragma vector aligned
#endif
    for (int i=0; i<n; i++)
      r[i]=i;
    return r;
  }

    /**
       For an integer array x, return the inverse of the running sum. 

       Let s[i] = \sum_{j=0}^{i} x[j] be the running sum of x. Then s's
       inverse r satisfies r[s[i]-1] = i.

       For instance, if x=(1,2,3,1), the running sum is (1,3,6,7), the
       inverse of which is (0,1,1,2,2,2,3).
    */
    template<class T, class A>
    inline array<T,A> gen_index(const array<T,A>& x)
    {
      array<T,A> r(sum(x),x.allocator());
      for (size_t i=0,p=0; i<x.size(); i++)
        for (int j=0; j<int(x[i]); j++,p++)
          r[p]=i;
      return r;
    }


  
    /// fill array with uniformly random numbers from [0,1)
    template <class F> void fillrand(array<F>& x);
    template <class F, class A> void fillrand(array<F, A>& x)
    {array<F> tmp(x.size()); fillrand(tmp); x=tmp;}
    /// fill array with gaussian random numbers from \f$N(0,1)\propto\exp(-x^2/2)\f$
    template <class F> void fillgrand(array<F>& x);
    template <class F, class A> void fillgrand(array<F,A>& x)
    {array<F> tmp(x.size()); fillgrand(tmp); x=tmp;}
    /// fill array with exponential random numbers \f$x\leftarrow-\ln\xi,\,\xi\in [0,1)\f$
    template <class F> void fillprand(array<F>& x);
    template <class F, class A> void fillprand(array<F,A>& x)
    {array<F> tmp(x.size()); fillprand(tmp); x=tmp;}
    
    /// fill with uniform numbers drawn from [0...\a max] without replacement
    void fill_unique_rand(array<int>& x, unsigned max);

    /// Multiplicative process \f$ a\leftarrow a(1+\xi s),\, \xi\in N(0,1)\f$
    template <class F> void lgspread( array<F>& a, const array<F>& s );
    /// Additive process \f$ a\leftarrow a+s\xi,\, \xi\in N(0,1)\f$
    template <class F> void gspread( array<F>& a, const array<F>& s );
    /// Multiplicative process \f$ a\leftarrow a(1+\xi s),\, \xi\in N(0,1)\f$
    template <class E, class F>
    void lgspread( array<F>& a, const E& s ) {lgspread(a,array<F>(s));}
    /// Additive process \f$ a\leftarrow a+\xi s,\, \xi\in N(0,1)\f$
    template <class E,class F>
    void gspread( array<F>& a, const E& s ) {gspread(a,array<F>(s));}

  /* ranking (sort) function */
  enum array_dir_t {upwards, downwards};

  template <class T>
  class Cmp
  {
    const array<T>& data;
    bool fwd;
  public:
    array<int> ranks;
    Cmp(const array<T>& data, bool fwd): data(data), fwd(fwd) {
      ranks=pcoord(data.size());
    }
    bool operator()(const int& i, const int& j)
    {
      if (fwd)
        return data[i] < data[j];
      else
        return data[j] < data[i];
    }
  };

  /// rank elements
  template <class T> typename
  enable_if<is_expression<T>, array<int> >::T
  rank(const T& x,  enum array_dir_t dir=upwards)
  {
    Cmp<typename T::value_type> cmp(x,dir==upwards);
    std::sort(cmp.ranks.begin(),cmp.ranks.end(),cmp);
    return cmp.ranks;
  }
    

  }  // namespace array_ns

  using array_ns::array;
  using array_ns::pcoord;

  extern urand array_urand;
  extern gaussrand array_grand;
} // namespace ecolab

namespace classdesc_access
{
  /* Definitions of pack, unpack for array classes */
  template <class T,class A>
  struct access_pack<ecolab::array_ns::array<T,A> >
  {
    template <class U>
    void operator()(classdesc::pack_t& targ, const classdesc::string& desc, 
                    U& arg)
    {
      pack(targ,desc,arg.size());
      pack(targ,desc,classdesc::is_array(),*arg.data(),1,arg.size());
    }
  };

  template <class T,class A>
  struct access_unpack<ecolab::array_ns::array<T,A> >
  {
    template <class U>
    void operator()(classdesc::unpack_t& targ, const classdesc::string& desc, 
                    U& arg)
    {
      size_t size;
      unpack(targ,desc,size);
      arg.resize(size);
      unpack(targ,desc,classdesc::is_array(),*arg.data(),1,size);
    }
  };
}

// standard is_sequence pack/unpack methods do not work here
template <class T,class A> void pack(classdesc::pack_t& targ, const classdesc::string& desc, const ecolab::array_ns::array<T,A>& arg)
{classdesc_access::access_pack<ecolab::array_ns::array<T>>()(targ,desc,arg);}
template <class T,class A> void unpack(classdesc::pack_t& targ, const classdesc::string& desc, ecolab::array_ns::array<T,A>& arg)
{classdesc_access::access_unpack<ecolab::array_ns::array<T>>()(targ,desc,arg);}



namespace classdesc
{
  template <class T,class A> 
  struct tn<ecolab::array<T,A> >
  {
    static std::string name()
    {return "ecolab::array<"+typeName<T>()+">";}
  };

  template <class T, class A> struct is_sequence<ecolab::array<T, A> >: public true_type {};

  template <class T, class A> struct Exclude<ecolab::array<T, A> >: public ExcludeClass<ecolab::array<T, A> >
  {
    template <class E>
    bool operator==(E x) const {return static_cast<const ecolab::array<T, A>&>(*this)==x;}
    template <class E>
    bool operator!=(E x) const {return !operator==(x);}
  };
}

#endif
