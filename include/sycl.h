/*
  @copyright Russell Standish 2026
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ECOLAB_SYCL_H
#define ECOLAB_SYCL_H
#ifdef SYCL_LANGUAGE_VERSION

#include <sycl/sycl.hpp>
#include "usmAlloc.h" // TODO - merge into this file.
#include "graphcode.h"

// we use some experimental features available in the OneAPI compiler
#ifndef __INTEL_LLVM_COMPILER
#error "EcoLab requires OneAPI compiler for some experimental functions"
#endif

namespace ecolab
{
  using sycl::ext::oneapi::experimental::printf;

  inline sycl::nd_item<1> syclItem() {return sycl::ext::oneapi::this_work_item::get_nd_item<1>();}
  inline sycl::group<1> syclGroup() {return sycl::ext::oneapi::this_work_item::get_work_group<1>();}
  inline sycl::sub_group syclSubGroup() {return sycl::ext::oneapi::this_work_item::get_sub_group();}

  extern bool syclQDestroyed;
  sycl::queue& syclQ();
  void* reallocSycl(void*,size_t);

  inline void groupBarrier() {sycl::group_barrier(syclGroup());}

  inline void syncThreads() {syclQ().wait_and_throw();}
    
  template <class T>
  struct SyclType: public T
  {
    template <class... A> SyclType(A... args): T(std::forward<A>(args)...) {}
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
  };

  template <>
  struct SyclType<size_t>
  {
    size_t data;
    operator size_t() const {return data;}
    operator size_t&() {return data;}
    size_t operator=(size_t x) {return data=x;}
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
  };

  template <class T>
  struct SyclType<T*>
  {
    T* data;
    operator T*() const {return data;}
    operator T*&() {return data;}
    T* operator=(T* x) {return data=x;}
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
  };
  
  template <class M>
  class DeviceType
  {
    SyclType<M>* const model;
  public:
    using element_type=M;
    template <class... A>
    DeviceType(A... args): model(new SyclType<M>(std::forward<A>(args)...)) {}
    DeviceType(const DeviceType& x): model(new SyclType<M>(*x.model)) {}
    DeviceType& operator=(const DeviceType& x) {*model=*x.model; return *this;}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
    // gcc incorrectly infers SyclType is polymorphic, which is quite plainly is not
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif
    ~DeviceType() {delete model;}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    M& operator*() {return *model;}
    const M& operator*() const {return *model;}
    M* operator->() {return model;}
    const M* operator->() const {return model;}
    operator bool() const {return true;} // always defined
  };

  template <class T, ecolab::USMAlloc UA>
  struct SyclQAllocator: public graphcode::Allocator<T>
  {
    SyclQAllocator(): graphcode::Allocator<T>(syclQ(), UA) {}
    template<class U> struct rebind {using other=SyclQAllocator;};
  };

}
#else // !SYCL
namespace ecolab
{
  inline void groupBarrier() {}
  inline void syncThreads() {}
}
#endif
#include "sycl.cd"
#endif
