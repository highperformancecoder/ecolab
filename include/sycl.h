/*
  @copyright Russell Standish 2026
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ECOLAB_SYCL_H
#define ECOLAB_SYCL_H
#include "usmAlloc.h" // TODO - merge into this file.
#include "graphcode.h"
#include <memory>
#include <utility>
#ifdef SYCL_LANGUAGE_VERSION

#include <sycl/sycl.hpp>

// we use some experimental features available in the OneAPI compiler
#ifndef __INTEL_LLVM_COMPILER
#error "EcoLab requires OneAPI compiler for some experimental functions"
#endif

#ifdef _OPENMP
#include <omp.h>
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

  inline void syncThreads() {syclQ().wait_and_throw();}

  /// SyclType is a pointer type, that when allocated via new is allocated from USM
  template <class T>
  struct SyclType: public T
  {
    T data;
    template <class... A> SyclType(A... args): data(std::forward<A>(args)...) {}
    operator T() const {return data;}
    operator T&() {return data;}
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
  };

  template <class T, ecolab::USMAlloc UA>
  struct SyclQAllocator: public graphcode::Allocator<T>
  {
    SyclQAllocator(): graphcode::Allocator<T>(syclQ(), UA) {}
    template<class U> struct rebind {using other=SyclQAllocator<U,UA>;};
  };
  
  // random numbers
  template <class E, ecolab::USMAlloc UA>
  struct SyclRandomEngine
  {
    std::vector<E,SyclQAllocator<E,UA>> rngs;
    static constexpr unsigned incr=1000000;
    SyclRandomEngine(size_t numRngs): rngs(numRngs) {}
    void seed() {for (auto& i: rngs) i.seed();}
    void seed(unsigned s) {
#ifdef __SYCL_DEVICE_ONLY__
      rngs[syclItem().get_global_linear_id() % rngs.size()].
        seed(s+syclItem().get_global_linear_id()*incr);
#else
      size_t numRngs=rngs.size();
#pragma omp parallel for
      for (unsigned j=0; j<numRngs; ++j)
        rngs[j].seed(s+j*incr);
#endif
    }
    unsigned operator()() {
#ifdef __SYCL_DEVICE_ONLY__
      return rngs[syclItem().get_global_linear_id() % rngs.size()]();
#elif defined(_OPENMP)
      return rngs[omp_get_thread_num()%rngs.size()]();
#else
      return rngs[0]();
#endif
    }
    unsigned min() const {return rngs[0].min();}
    unsigned max() const {return rngs[0].max();}
  };
}
#else // !SYCL
#include "non-sycl.h"
#endif

namespace ecolab
{
  /// Used to declare an object in unified shared memory, accessible on GPU device
  /// Smart pointer semantics, but never invalid
  template <class M>
  class DeviceType
  {
    M* const model;
  public:
    using element_type=M;
    template <class... A>
#ifdef SYCL_LANGUAGE_VERSION
    DeviceType(A... args): model(sycl::malloc_shared<M>(1,syclQ()))
      {new (model) M(std::forward<A>(args)...);}
#else
    DeviceType(A... args): model(new M(std::forward<A>(args)...)) {}
#endif
    DeviceType& operator=(const DeviceType& x) {*model=*x.model; return *this;}
    ~DeviceType() {
#ifdef SYCL_LANGUAGE_VERSION
      model->~M();
      sycl::free(model,syclQ());
#else
      delete model;
#endif
    }
    M& operator*() {return *model;}
    const M& operator*() const {return *model;}
    M* operator->() {return model;}
    const M* operator->() const {return model;}
    operator bool() const {return true;} // always defined
  };

#ifndef __SYCL_DEVICE_ONLY__
  template <class T> using LocalAllocator=std::allocator<T>;
#endif
  
  inline void groupBarrier() {
#ifdef __SYCL_DEVICE_ONLY__
    sycl::group_barrier(syclGroup());
#endif
  }

  inline bool groupLeader() {
#ifdef __SYCL_DEVICE_ONLY__
    return syclGroup().leader();
#endif
    return true;
  }

  inline bool onDevice() {
#ifdef __SYCL_DEVICE_ONLY__
    return true;
#endif
    return false;
  }
}


#define CLASSDESC_RESTProcess___ecolab__DeviceType_M_
#define CLASSDESC_json_pack___ecolab__DeviceType_M_
#define CLASSDESC_json_unpack___ecolab__DeviceType_M_
#define CLASSDESC_pack___ecolab__DeviceType_M_
#define CLASSDESC_unpack___ecolab__DeviceType_M_
#include "sycl.cd"
#endif
