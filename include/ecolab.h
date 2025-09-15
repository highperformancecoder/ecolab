/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief master include file for EcoLab projects
*/
#ifndef ECOLAB_H
#define ECOLAB_H

#ifdef SYCL_LANGUAGE_VERSION
#include <dpct/dpct.hpp>
#define DPCT_COMPATIBILITY_TEMP 600
#include "Utility.dp.hpp"
#include "device/Ouroboros.dp.hpp"
#include "device/Ouroboros_impl.dp.hpp"
#include "InstanceDefinitions.dp.hpp"
#include "device/MemoryInitialization.dp.hpp"
#ifdef __INTEL_LLVM_COMPILER
template <typename... Args>
void syclPrintf(Args... args) {sycl::ext::oneapi::experimental::printf(args...);}
inline sycl::nd_item<1> syclItem() {return sycl::ext::oneapi::this_work_item::get_nd_item<1>();}
inline sycl::group<1> syclGroup() {return sycl::ext::oneapi::this_work_item::get_work_group<1>();}
inline sycl::sub_group syclSubGroup() {return sycl::ext::oneapi::this_work_item::get_sub_group();}
#else
#define syclPrintf(...)
#endif
#endif

#include <stdlib.h>
#include "pythonBuffer.h"
#include "graphcode.h"
#include "usmAlloc.h"

// mpi.h must appear before any standard library stuff
#ifdef MPI_SUPPORT
#include <mpi.h>
#endif

/* list of definitions to enable the "hacked" (by insert-friend)
   versions of standard headers to compile */

namespace classdesc
{
  class pack_t;
//class unpack_t;
  typedef pack_t unpack_t;
  class TCL_obj_t;
}

#define THROW_PTR_EXCEPTION  //Allows more generously for types containing pointers

#ifdef MEMDEBUG
#define Realloc Realloc
namespace ecolab {char *Realloc(char *p, size_t sz);}
using ecolab::Realloc;
#endif
#include "pack.h"
typedef classdesc::string eco_string;

#include "classdesc_access.h"

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <isa_base.h>

#include "eco_strstream.h"
#include "random.h"

namespace ecolab
{
  using namespace classdesc;
  
  /* these are defined to default values, even if MPI is false */
  /// MPI process ID and number of processes
  unsigned myid();
  unsigned nprocs();

  /// adds the EcoLab module path to the python interpreter
  int addEcoLabPath();
  /// initialisation of the parallel type object
  void registerParallel();

  using classdesc::pack_t;
  using classdesc::unpack_t;
  using classdesc::xdr_pack;
  using classdesc::isa_t;
  using classdesc::is_array;

  template <class M> struct Model
  {
    /// checkpoint the model to \a fileName
    void checkpoint(const char* fileName);
    /// restart the model from checkpoint saved in \a fileName
    void restart(const char* fileName);
  };

  struct OnExit
  {
    std::function<void()> f;
    OnExit(const std::function<void()>& f): f(f) {}
    ~OnExit() {f();}
  };
  
#ifdef SYCL_LANGUAGE_VERSION
  extern bool syclQDestroyed;
  sycl::queue& syclQ();
  void* reallocSycl(void*,size_t);
#endif

  inline void groupBarrier()
  {
#ifdef __SYCL_DEVICE_ONLY__
    sycl::group_barrier(syclGroup());
#endif
  }
  
  template <class T>
  struct SyclType: public T
  {
    template <class... A> SyclType(A... args): T(std::forward<A>(args)...) {}
#ifdef SYCL_LANGUAGE_VERSION
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
#endif
  };

  template <>
  struct SyclType<size_t>
  {
    size_t data;
    operator size_t() const {return data;}
    operator size_t&() {return data;}
    size_t operator=(size_t x) {return data=x;}
#ifdef SYCL_LANGUAGE_VERSION
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
#endif
  };

  template <class T>
  struct SyclType<T*>
  {
    T* data;
    operator T*() const {return data;}
    operator T*&() {return data;}
    T* operator=(T* x) {return data=x;}
#ifdef SYCL_LANGUAGE_VERSION
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
#endif
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
#ifdef SYCL_LANGUAGE_VERSION
    SyclQAllocator(): graphcode::Allocator<T>(syclQ(), UA) {}
#endif
    template<class U> struct rebind {using other=SyclQAllocator;};
  };
  
  struct CellBase
  {
#ifdef SYCL_LANGUAGE_VERSION
    Ouro::SyclDesc<1>* desc=nullptr;
    using MemAllocator=Ouro::MultiOuroPQ;
    MemAllocator* memAlloc=nullptr;
    const sycl::stream* out=nullptr;
    size_t idx() const {
      if (desc) return desc->item.get_global_linear_id();
      else return 0;
    }
    template <class T> class CellAllocator
    {
    public:
      Ouro::SyclDesc<1>*const* desc=nullptr;
      MemAllocator*const* memAlloc=nullptr;
      template <class U> friend class Allocator;
      CellAllocator()=default;
      CellAllocator(Ouro::SyclDesc<1>* const& desc, MemAllocator*const& memAlloc):
        desc(&desc), memAlloc(&memAlloc) {
      }
      template <class U> CellAllocator(const CellAllocator<U>& x):
        desc(x.desc), memAlloc(x.memAlloc) {}
      T* allocate(size_t sz) {
#ifdef  __SYCL_DEVICE_ONLY__
        if (memAlloc && *memAlloc && desc && *desc)  {
          auto r=reinterpret_cast<T*>((*memAlloc)->malloc(**desc,sz*sizeof(T)));
          if (!r) syclPrintf("Mem allocation failed\n");
          return r;
        }
        return nullptr; // TODO raise an error??
#else
        return sycl::malloc_shared<T>(sz,syclQ());
#endif
      }
      void deallocate(T* p,size_t) {
#ifdef  __SYCL_DEVICE_ONLY__
        if (memAlloc && *memAlloc && desc && *desc)
          (*memAlloc)->free(**desc,p);
#else
        sycl::free(p,syclQ());
#endif
      }
      bool operator==(const CellAllocator& x) const {return desc==x.desc && memAlloc==x.memAlloc;}
    };
    template <class T> CellAllocator<T> allocator() const {
      return CellAllocator<T>(desc,memAlloc);
    }
#else
    template <class T> using CellAllocator=std::allocator<T>;
    template <class T> CellAllocator<T> allocator() const {return CellAllocator<T>();}
    size_t m_idx=0; // stash the position within the local node vector here
    size_t idx() const {return m_idx;}
#endif
  };

  template <class T>
  void printAllocator(const char* prefix, const T&) {}
  
  template <class T>
  void printAllocator(const char* prefix, const CellBase::CellAllocator<T>& x)
  {syclPrintf("%s: allocator.desc=%p memAlloc=%p\n",prefix,x.desc,x.memAlloc);}
  
  
  class SyclGraphBase
  {
  protected:
#ifdef SYCL_LANGUAGE_VERSION
    using MemAllocator=CellBase::MemAllocator;
    MemAllocator *sharedMemAlloc=nullptr, *deviceMemAlloc=nullptr;
    SyclGraphBase() {
      // for now, do not use on-device dynamic allocation
      sharedMemAlloc=sycl::malloc_shared<MemAllocator>(1, syclQ());
      new(sharedMemAlloc) MemAllocator;
      deviceMemAlloc=sycl::malloc_shared<MemAllocator>(1, syclQ());
      new(deviceMemAlloc) MemAllocator;
      sharedMemAlloc->initialize(syclQ(), sycl::usm::alloc::shared, 512ULL * 1024ULL * 1024ULL); // TODO - expose this Python?
      deviceMemAlloc->initialize(syclQ(), sycl::usm::alloc::device, 512ULL * 1024ULL * 1024ULL); // TODO - expose this Python?
    }
    ~SyclGraphBase() {
      sharedMemAlloc->~MemAllocator();
      sycl::free(sharedMemAlloc,syclQ());
      deviceMemAlloc->~MemAllocator();
      sycl::free(deviceMemAlloc,syclQ());
    }
    // deleted because we're managing a resource here
    SyclGraphBase(const SyclGraphBase&)=delete;
    void operator=(const SyclGraphBase&)=delete;
#endif      
  };

  template <class Cell> struct EcolabGraph:
    public Exclude<SyclGraphBase>, public graphcode::Graph<Cell>
  {
#ifdef SYCL_LANGUAGE_VERSION
    EcolabGraph(): graphcode::Graph<Cell>
      (graphcode::Allocator<Cell>(syclQ(),sycl::usm::alloc::shared),
       graphcode::Allocator<graphcode::ObjRef>(syclQ(),sycl::usm::alloc::shared),
       graphcode::Allocator<graphcode::ObjectPtr<Cell>>(syclQ(),sycl::usm::alloc::shared)) {}
    ~EcolabGraph() {syncThreads();}
#endif
    /// apply a functional to all local cells of this processor in parallel
    /// @param f 
    template <class F>
    void forAll(F f) {
#ifdef SYCL_LANGUAGE_VERSION
      static size_t workGroupSize=syclQ().get_device().get_info<sycl::info::device::max_work_group_size>();
      size_t range=this->size()/workGroupSize;
      if (range*workGroupSize < this->size()) ++range;
      syclQ().submit([&](auto& h) {
#ifndef NDEBUG
        sycl::stream out(1000000,1000,h);
#endif
        h.parallel_for(sycl::nd_range<1>(range*workGroupSize, workGroupSize), [=,this](auto i) {
          auto idx=i.get_global_linear_id();
          if (idx<this->size()) {
            auto& cell=*(*this)[idx]->template as<Cell>();
            Ouro::SyclDesc<> desc(i,{});
            cell.desc=&desc;
#ifndef NDEBUG
            cell.out=&out;
#endif
            f(cell);
            cell.desc=nullptr;
            cell.out=nullptr;
          }
        });
      });
#else
      auto sz=this->size();
#ifdef _OPENMP
#pragma omp parallel for
#endif
      for (size_t idx=0; idx<sz; ++idx) {
        auto& cell=*(*this)[idx]->template as<Cell>();
        cell.m_idx=idx;
        f(cell);
      }
#endif
    }
    
    /// apply a functional to all local cells of this processor in
    /// parallel, where each cell is allocated SIMD parallel computer
    /// (known as a Group in SYCL, or Block in CUDA
    /// @param f - functional that takes a \a Cell or \a const \a Cell argument
    template <class F>
    void groupedForAll(F f) {
#ifdef SYCL_LANGUAGE_VERSION
      static size_t workGroupSize=32;//syclQ().get_device().get_info<sycl::info::device::max_work_group_size>();
      syclQ().submit([&](auto& h) {
#ifndef NDEBUG
        sycl::stream out(1000000,1000,h);
#endif
        h.parallel_for(sycl::nd_range<1>(this->size()*workGroupSize, workGroupSize), [=,this](auto i) {
          auto idx=i.get_group_linear_id();
          if (idx<this->size()) {
            auto& cell=*(*this)[idx]->template as<Cell>();
            Ouro::SyclDesc<> desc(i,{});
            cell.desc=&desc;
#ifndef NDEBUG
            cell.out=&out;
#endif
            f(cell);
            cell.desc=nullptr;
            cell.out=nullptr;
          }
        });
      });
#else
      forAll(f);
#endif
    }

    void syncThreads() {
#ifdef SYCL_LANGUAGE_VERSION
      syclQ().wait();
#endif
    }
    
    template <class T, class F> T max(T x, F f) {
#ifdef SYCL_LANGUAGE_VERSION
      DeviceType<T> r(x);
      syclQ().parallel_for(this->size(),[=,x=&*r,this](auto i) {
          sycl::atomic_ref<T,sycl::memory_order::relaxed,sycl::memory_scope::device> ax(*x);
          auto& cell=*(*this)[i]->template as<Cell>();
          ax.fetch_max(f(cell));
        }).wait();
      return *r;
#else
      auto sz=this->size();
#ifdef _OPENMP
#pragma omp parallel for reduction(max:x)
#endif
      for (size_t idx=0; idx<sz; ++idx) {
        auto& cell=*(*this)[idx]->template as<Cell>();
        cell.m_idx=idx;
        x=std::max(x,f(cell));
      }
      return x;
#endif
    }
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
        ref().~T();
    }
    T& ref() {return reinterpret_cast<T&>(**buffer);}
#endif
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

}

namespace classdesc
{
  template <class M> struct is_smart_ptr<ecolab::DeviceType<M>>: public true_type {};
  template <class T> struct is_char: public false_type {};
  template <> struct is_char<char>: public true_type {};
  template <> struct is_char<wchar_t>: public true_type {};

#ifdef SYCL_LANGUAGE_VERSION
  /// classdesc support for stringifying usm_alloc
  namespace   {
    template <> EnumKey enum_keysData< ::sycl::usm::alloc >::keysData[]=
      {
        {"host",int(::sycl::usm::alloc::host)},
        {"device",int(::sycl::usm::alloc::device)},
        {"shared",int(::sycl::usm::alloc::shared)},
        {"unknown",int(::sycl::usm::alloc::unknown)}
      };
    template <> EnumKeys< ::sycl::usm::alloc > enum_keysData< ::sycl::usm::alloc >::keys(enum_keysData< ::sycl::usm::alloc >::keysData,sizeof(enum_keysData< ::sycl::usm::alloc >::keysData)/sizeof(enum_keysData< ::sycl::usm::alloc >::keysData[0]));
    template <> int enumKey< ::sycl::usm::alloc >(const std::string& x){return int(enum_keysData< ::sycl::usm::alloc >::keys(x));}
    template <> std::string enumKey< ::sycl::usm::alloc >(int x){return enum_keysData< ::sycl::usm::alloc >::keys(x);}
  }
#endif

}

namespace std
{
  template <class T>
  typename classdesc::enable_if<
    classdesc::Not<classdesc::is_char<typename std::remove_cv<T>::type>>,
    std::ostream&>::T
  operator<<(std::ostream& o, T* p)
  {return o<<std::hex<<std::size_t(p);}
}

#ifdef MPI_SUPPORT
#include <classdescMP.h>
namespace ecolab
{
  using classdesc::MPIbuf;
  using classdesc::MPIbuf_array;
  using classdesc::bcast;
  using classdesc::send;
  using classdesc::isend;
}
#endif

// classdesc omissions
#define CLASSDESC_pack___ecolab__CellBase
#define CLASSDESC_unpack___ecolab__CellBase
#define CLASSDESC_pack___ecolab__SyclGraphBase
#define CLASSDESC_unpack___ecolab__SyclGraphBase
#define CLASSDESC_pack___ecolab__CellBase__CellAllocator_T_
#define CLASSDESC_unpack___ecolab__CellBase__CellAllocator_T_
#define CLASSDESC_json_pack___ecolab__CellBase
#define CLASSDESC_json_unpack___ecolab__CellBase
#define CLASSDESC_json_pack___ecolab__CellBase__CellAllocator_T_
#define CLASSDESC_json_unpack___ecolab__CellBase__CellAllocator_T_
#define CLASSDESC_RESTProcess___ecolab__CellBase
#define CLASSDESC_RESTProcess___ecolab__CellBase__CellAllocator_T_

#include "ecolab.cd"
#endif  /* ECOLAB_H */
