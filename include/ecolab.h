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
#endif

#include <stdlib.h>
#include "pythonBuffer.h"
#include "graphcode.h"

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

#ifdef SYCL_LANGUAGE_VERSION
  sycl::queue& syclQ();
  void* reallocSycl(void*,size_t);
#endif
  
  template <class T>
  struct SyclType: public T
  {
#ifdef SYCL_LANGUAGE_VERSION
    void* operator new(size_t s) {return reallocSycl(nullptr,s);}
    void operator delete(void* p) {reallocSycl(p,0);}
    void* operator new[](size_t s) {return reallocSycl(nullptr,s);}
    void operator delete[](void* p) {reallocSycl(p,0);}
#endif
  };

  template <class M>
  struct DeviceType
  {
    using element_type=M;
    SyclType<M>* const model=new SyclType<M>;
    DeviceType()=default;
    DeviceType(const DeviceType& x) {*this=x;}
    DeviceType& operator=(const DeviceType& x) {*model=x.*model; return *this;}
    ~DeviceType() {delete model;}
    M& operator*() {return *model;}
    const M& operator*() const {return *model;}
    M* operator->() {return model;}
    const M* operator->() const {return model;}
    operator bool() const {return true;} // always defined
  };

  struct CellBase
  {
#ifdef SYCL_LANGUAGE_VERSION
    Ouro::SyclDesc<1>* desc=nullptr;
    using MemAllocator=Ouro::MultiOuroPQ;
    MemAllocator* memAlloc=nullptr;
    const sycl::stream* out=nullptr;
    template <class T> class Allocator
    {
    public:
      Ouro::SyclDesc<1>*const* desc=nullptr;
      MemAllocator*const* memAlloc;
      template <class U> friend class Allocator;
      Allocator()=default;
      Allocator(Ouro::SyclDesc<1>* const& desc, MemAllocator*const& memAlloc):
        desc(&desc), memAlloc(&memAlloc) {
      }
      template <class U> Allocator(const Allocator<U>& x):
        desc(x.desc) {}
      T* allocate(size_t sz) {
        if (memAlloc && *memAlloc && desc && *desc) 
          return reinterpret_cast<T*>((*memAlloc)->malloc(**desc,sz*sizeof(T)));
        else
          return nullptr; // TODO raise an error??
      }
      void deallocate(T* p,size_t) {
        if (memAlloc && *memAlloc && desc && *desc)
          (*memAlloc)->free(**desc,p);
      }
      bool operator==(const Allocator& x) const {return desc==x.desc && memAlloc==x.memAlloc;}
    };
    template <class T> Allocator<T> allocator() const {
      return Allocator<T>(desc,memAlloc);
    }
#else
    template <class T> using Allocator=std::allocator<T>;
    template <class T> Allocator<T> allocator() const {return Allocator<T>();}
#endif
  };

  class SyclGraphBase
  {
  protected:
#ifdef SYCL_LANGUAGE_VERSION
    using MemAllocator=CellBase::MemAllocator;
    MemAllocator* memAlloc;
    SyclGraphBase() {
      memAlloc=sycl::malloc_shared<MemAllocator>(1, syclQ());
      new(memAlloc) MemAllocator;
      memAlloc->initialize(syclQ(), sycl::usm::alloc::shared, 512ULL * 1024ULL * 1024ULL); // TODO - expose this Python?
    }
    ~SyclGraphBase() {
      memAlloc->~MemAllocator();
      sycl::free(memAlloc,syclQ());
    }
    // deleted because we're managing a resource here
    SyclGraphBase(const SyclGraphBase&)=delete;
    void operator=(const SyclGraphBase&)=delete;
#endif      
  };

  // This class exists, because we need a default constructor, and
  // usm_allocator needs to be initialised with a device context
#ifdef SYCL_LANGUAGE_VERSION
  template <class T>
  struct CellAllocator: public sycl::usm_allocator<T,sycl::usm::alloc::shared>
  {
    CellAllocator(): sycl::usm_allocator<T,sycl::usm::alloc::shared>(syclQ()) {}
    template<class U> constexpr CellAllocator(const CellAllocator<U>& x) noexcept:
      sycl::usm_allocator<T,sycl::usm::alloc::shared>(x) {}
    template<class U> struct rebind {using other=CellAllocator<U>;};
  };
#else
  template <class T> struct CellAllocator: std::allocator<T>
  {
    CellAllocator()=default;
    template<class U> constexpr CellAllocator(const CellAllocator<U>& x) noexcept:
      std::allocator<T>(x) {}
    template<class U> struct rebind {using other=CellAllocator<U>;};
  };
#endif
  
  
  template <class Cell> struct EcolabGraph:
    public Exclude<SyclGraphBase>, public graphcode::Graph<Cell, CellAllocator>
  {
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
            cell.memAlloc=this->memAlloc;
#ifndef NDEBUG
            cell.out=&out;
#endif
            f(cell);
            cell.desc=nullptr;
            cell.out=nullptr;
          }
        });
      });
      syclQ().wait();
#else
      for (auto& i: *this) f(*i->template as<Cell>());
#endif
    }
  };

}

namespace classdesc
{
  template <class M> struct is_smart_ptr<ecolab::DeviceType<M>>: public true_type {}; 
}

namespace classdesc_access
{
  template <class T>
  struct access_pack<ecolab::CellAllocator<T>>: public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <class T>
  struct access_unpack<ecolab::CellAllocator<T>>: public classdesc::NullDescriptor<classdesc::pack_t> {};
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
#define CLASSDESC_pack___ecolab__CellBase__Allocator_T_
#define CLASSDESC_unpack___ecolab__CellBase__Allocator_T_
#define CLASSDESC_pack___ecolab__CellAllocator_T_
#define CLASSDESC_unpack___ecolab__CellAllocator_T_
#define CLASSDESC_json_pack___ecolab__CellBase
#define CLASSDESC_json_unpack___ecolab__CellBase
#define CLASSDESC_json_pack___ecolab__CellBase__Allocator_T_
#define CLASSDESC_json_unpack___ecolab__CellBase__Allocator_T_
#define CLASSDESC_json_pack___ecolab__CellAllocator_T_
#define CLASSDESC_json_unpack___ecolab__CellAllocator_T_
#define CLASSDESC_RESTProcess___ecolab__CellBase
#define CLASSDESC_RESTProcess___ecolab__CellBase__Allocator_T_
#define CLASSDESC_RESTProcess___ecolab__CellAllocator_T_

#include "ecolab.cd"
#endif  /* ECOLAB_H */
