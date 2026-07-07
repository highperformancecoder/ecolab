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
    template<class U> struct rebind {using other=SyclQAllocator;};
  };
  
  template <class T>
  class GroupLocal
  {
#ifdef __SYCL_DEVICE_ONLY__
    using BufferType=char[sizeof(T)];
    using LocalBufferType=decltype(sycl::ext::oneapi::group_local_memory_for_overwrite<BufferType>(syclGroup()));
    LocalBufferType buffer=sycl::ext::oneapi::group_local_memory_for_overwrite<BufferType>(syclGroup());
  public:
    template <class... Args>
    GroupLocal(Args&&... args) {
      if (syclGroup().leader())
        new (*buffer) T(std::forward<Args>(args)...);
      sycl::group_barrier(syclGroup());
    }
    ~GroupLocal() {
      sycl::group_barrier(syclGroup());
      if (syclGroup().leader())
        (**this).~T();
    }
    T& operator*() {return reinterpret_cast<T&>(**buffer);}
#else
    T buffer;
  public:
    template <class... Args>
    GroupLocal(Args&&... args): buffer(std::forward<Args>(args)...) {}
    T& operator*() {return buffer;}
#endif
    T* operator->() {return &**this;}
  };

#ifdef __SYCL_DEVICE_ONLY__
  template <class T, size_t N, size_t M>
  T* getGroupBuffer()
  {
    if constexpr (N<=M)
      return *sycl::ext::oneapi::group_local_memory_for_overwrite<T[1<<N]>(syclGroup());
    assert(false && "Group buffer limit exceeded");
    return nullptr;
  }  
  
  /// Return a buffer in group local memory, of at least \a n Ts.
  /// @tparam T element type of buffer
  /// @tparam N maximum buffer size supported is 2^N.
  /// This method can only be called in kernel code
  /// Increasing N hurts performance, or may exceed system limits.
  /// Individual elements are default initialised
  /// Buffer is destroyed once all threads in group have completed (not at scope exit)
  template <class T, size_t N>
  T* groupBuffer(size_t n) {
    // work out power of two >= n
    switch (sizeof(size_t)*8-sycl::clz(n-1))
      {
      case 0: return getGroupBuffer<T,0,N>();
      case 1: return getGroupBuffer<T,1,N>();
      case 2: return getGroupBuffer<T,2,N>();
      case 3: return getGroupBuffer<T,3,N>();
      case 4: return getGroupBuffer<T,4,N>();
      case 5: return getGroupBuffer<T,5,N>();
      case 6: return getGroupBuffer<T,6,N>();
      case 7: return getGroupBuffer<T,7,N>();
      case 8: return getGroupBuffer<T,8,N>();
      case 9: return getGroupBuffer<T,9,N>();
      case 10: return getGroupBuffer<T,10,N>();
      case 11: return getGroupBuffer<T,11,N>();
      case 12: return getGroupBuffer<T,12,N>();
      case 13: return getGroupBuffer<T,13,N>();
      case 14: return getGroupBuffer<T,14,N>();
      case 15: return getGroupBuffer<T,15,N>();
      case 16: return getGroupBuffer<T,16,N>();
      default: assert(false && "Group buffer limit exceeded"); return nullptr;
      }
  }
#endif

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
      return rngs[omp_get_thread_num()%numRngs]();
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
//#if defined(__GNUC__) && !defined(__clang__)
//#pragma GCC diagnostic push
//    // gcc incorrectly infers SyclType is polymorphic, which is quite plainly is not
//#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
//#endif
    ~DeviceType() {
#ifdef SYCL_LANGUAGE_VERSION
      model->~M();
      sycl::free(model,syclQ());
#else
      delete model;
#endif
    }
//#if defined(__GNUC__) && !defined(__clang__)
//#pragma GCC diagnostic pop
//#endif
    M& operator*() {return *model;}
    const M& operator*() const {return *model;}
    M* operator->() {return model;}
    const M* operator->() const {return model;}
    operator bool() const {return true;} // always defined
  };

}


#define CLASSDESC_RESTProcess___ecolab__DeviceType_M_
#define CLASSDESC_json_pack___ecolab__DeviceType_M_
#define CLASSDESC_json_unpack___ecolab__DeviceType_M_
#define CLASSDESC_pack___ecolab__DeviceType_M_
#define CLASSDESC_unpack___ecolab__DeviceType_M_
#include "sycl.cd"
#endif
