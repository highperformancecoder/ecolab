/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief support for Java signatures
*/
#ifndef JAVACLASSDESCRIPTOR_H
#define JAVACLASSDESCRIPTOR_H
#include <string>
#include "function.h"
#include "classdesc.h"
#include <exception>

namespace classdesc
{
  struct sig_error: public std::exception
  {
    const char* what() const throw() {return "Invalid java signature";}
  };

  /// Descriptor object
  template <class T>  struct Descriptor;// {static std::string d();};

  template <> struct Descriptor<void> {static std::string d() {return "V";}};
  template <> struct Descriptor<bool> {static std::string d() {return "Z";}};
  template <> struct Descriptor<signed char> {static std::string d() {return "C";}};
  template <> struct Descriptor<unsigned char> {static std::string d() {return "C";}};
  template <> struct Descriptor<short> {static std::string d() {return "S";}};
  template <> struct Descriptor<unsigned short> {static std::string d() {return "S";}};
  template <> struct Descriptor<int> {static std::string d() {return "I";}};
  template <> struct Descriptor<unsigned int> {static std::string d() {return "I";}};
  template <> struct Descriptor<long> {static std::string d() {
    if (sizeof(long)==sizeof(int)) return "I";
    else return "J";}};
  template <> struct Descriptor<unsigned long> {static std::string d() {
    if (sizeof(long)==sizeof(int)) return "I";
    else return "J";}};
#ifdef HAVE_LONGLONG
  template <> struct Descriptor<long long> {static std::string d() {return "J";}};
  template <> struct Descriptor<unsigned long long> {static std::string d() {return "J";}};
#endif
  template <> struct Descriptor<float> {static std::string d() {return "F";}};
  template <> struct Descriptor<double> {static std::string d() {return "D";}};

  template <> struct Descriptor<std::string> {static std::string d() {return "Ljava/lang/String;";}};

  template <class T> struct Descriptor<const T> 
  {static std::string d() {return Descriptor<T>::d();}};

  template <class T> struct Descriptor<T&> 
  {static std::string d() {return Descriptor<T>::d();}};

  template <class T> struct Descriptor<const T&> 
  {static std::string d() {return Descriptor<T>::d();}};

  template <class F, int i> struct arg_desc;

  /**
     \a F is a functional, \a is a type description class,
     TD<T>::D is a string describing T
  */
  template <class F>
  struct arg_desc<F,0>
  {
    static std::string d() {return std::string();}
  };

  template <class F, int i> 
  struct arg_desc
  {
    static std::string d() {
      return arg_desc<F,i-1>::d() + Descriptor<typename functional::Arg<F,i>::T>::d();
    }
  };

    ///Return a concatenated string of argument descriptors
  template <class F>
  std::string arg_description(F f) 
  {return arg_desc<F, functional::Arity<F>::V>::d();}

  template <class M>  
  typename enable_if<functional::is_function<M>, std::string>::T
  descriptor(dummy<0> d=0) 
  {
    return std::string("(") + arg_desc<M, functional::Arity<M>::V>::d() +
      ")" + Descriptor<typename functional::Return<M>::T>::d();
  }

  // return Java signature string for type T
  template <class S>  
  typename enable_if<is_fundamental<S>, std::string>::T
  descriptor(dummy<2> d=0) {return Descriptor<S>::d();}


}
#endif
