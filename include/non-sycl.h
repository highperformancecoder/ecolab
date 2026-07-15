/*
  @copyright Russell Standish 2026
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/
#ifndef ECOLAB_NON_SYCL_H
#define ECOLAB_NON_SYCL_H
namespace ecolab
{
  template <class T, ecolab::USMAlloc U> struct SyclQAllocator: public std::allocator<T> {};
  
  inline void syncThreads() {}

  /// Non SYCL version of SyclType
  template <class T>
  struct SyclType
  {
    T data;
    template <class... A> SyclType(A... args): data(std::forward<A>(args)...) {}
    operator T() const {return data;}
    operator T&() {return data;}
  };
  
  template <class T> struct GroupLocal: public std::unique_ptr<T>
  {
  };
  
  template <class E, ecolab::USMAlloc UA>
  struct SyclRandomEngine: public E {};

}
#endif
