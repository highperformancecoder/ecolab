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

#include "sycl.h"
#ifdef SYCL_LANGUAGE_VERSION
#include "DeviceAllocator.h"
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
  //  class TCL_obj_t;
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

  /// size of work groups when running on GPU
  extern unsigned workGroupSize;
  
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
  
  class GlobalDeallocateKernelTag;
  struct CellBase
  {
    size_t m_idx=0; // stash the position within the local node vector here
    size_t idx() const {return m_idx;}
  };

  template <class Cell> struct EcolabGraph: public graphcode::Graph<Cell>
  {
#ifdef SYCL_LANGUAGE_VERSION
    EcolabGraph(): graphcode::Graph<Cell>
      (graphcode::Allocator<Cell>(syclQ(),sycl::usm::alloc::shared),
       graphcode::Allocator<graphcode::ObjRef>(syclQ(),sycl::usm::alloc::shared),
       graphcode::Allocator<graphcode::ObjectPtr<Cell>>(syclQ(),sycl::usm::alloc::shared)) {}
    ~EcolabGraph() {syncThreads();}
#endif
    /// apply a functional to all local cells of this processor using the host processor
    /// @param f 
    template <class F>
    void hostForAll(F f) {
      auto sz=this->size();
#ifdef _OPENMP
#pragma omp parallel for
#endif
      for (size_t idx=0; idx<sz; ++idx) {
        auto& cell=*(*this)[idx]->template as<Cell>();
        f(cell,idx);
      }
    }
    
    /// apply a functional to all local cells of this processor in parallel
    /// @param f 
    template <class F>
    void forAll(F f) {
#ifdef SYCL_LANGUAGE_VERSION
      static size_t workGroupSize=syclQ().get_device().get_info<sycl::info::device::max_work_group_size>();
      size_t range=this->size()/workGroupSize;
      if (range*workGroupSize < this->size()) ++range;
      syclQ().submit([&](auto& h) {
        h.parallel_for(sycl::nd_range<1>(range*workGroupSize, workGroupSize), [=,this](auto i) {
          auto idx=i.get_global_linear_id();
          if (idx<this->size()) {
            auto& cell=*(*this)[idx]->template as<Cell>();
            f(cell,idx);
          }
        });
      });
#else
      hostForAll(f);
#endif
    }
    
    /// apply a functional to all local cells of this processor in
    /// parallel, where each cell is allocated SIMD parallel computer
    /// (known as a Group in SYCL, or Block in CUDA
    /// @param f - functional that takes a \a Cell or \a const \a Cell argument
    template <class F>
    void groupedForAll(F f) {
#ifdef SYCL_LANGUAGE_VERSION
      auto dev=syclQ().get_device();
      // 1. Max threads per single work-group
      size_t max_wg_size = dev.get_info<sycl::info::device::max_work_group_size>();

      // 2. Hardware SIMD width (Sub-group size, usually 8, 16, or 32 on Intel)
      std::vector<size_t> sg_sizes = dev.get_info<sycl::info::device::sub_group_sizes>();
      size_t native_sg_size = sg_sizes.back(); // Usually 16 or 32 is best for compute

      // 3. Total physical SLM available per hardware compute unit (DSS)
      size_t max_slm_size = dev.get_info<sycl::info::device::local_mem_size>(); 

      // 4. Maximum number of compute units (Execution units / DSS count)
      uint32_t max_compute_units = dev.get_info<sycl::info::device::max_compute_units>();

//      if (workGroupSize > max_wg_size) 
//        workGroupSize = max_wg_size; // Fallback for limited devices
//      
//      // Ensure it aligns perfectly with hardware SIMD lanes
//      workGroupSize = (workGroupSize / native_sg_size) * native_sg_size;

      size_t wg_per_compute_unit = max_slm_size / LocalAllocatorSize;
      // To maximize latency hiding, it's often beneficial to double or triple this 
      // so the GPU can switch to a waiting wave while another wave is blocked by a barrier.
      size_t num_work_groups = max_compute_units * wg_per_compute_unit;

      num_work_groups=std::min(num_work_groups,this->size());
      //std::cout<<max_slm_size<<" max_slm_size "<<max_compute_units<<" max_compute_units "<<num_work_groups<<" work groups of "<<workGroupSize<<" threads"<<std::endl;

      DeviceType<int> fatalError(0);
      for (size_t cellStart=0; cellStart<this->size(); cellStart+=num_work_groups)
        syclQ().submit([&](auto& h) {
          h.parallel_for(sycl::nd_range<1>(num_work_groups*workGroupSize, workGroupSize), [=,this,fatalError=&*fatalError](auto i) {
            if (syclGroup().leader()) {
              localAllocatorBuffer().next = 0;
            } // reset local memory allocation
            sycl::group_barrier(syclGroup()); 

            auto idx = cellStart+i.get_group_linear_id();
            auto stride = i.get_group_range(0);
            if (idx < this->size()) {
              auto& cell=*(*this)[idx]->template as<Cell>();
              f(cell,idx);
            }
            // flag fatal error to throw afterwards.
            if (fatalErrorFlag())
              sycl::atomic_ref<int,sycl::memory_order::relaxed,sycl::memory_scope::device>(*fatalError).fetch_or(1);
          });
        });
      syclQ().wait_and_throw();
      if (*fatalError)
        throw std::runtime_error("Local Allocator Exhausted");
#else
      hostForAll(f);
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
        x=std::max(x,f(cell));
      }
      return x;
#endif
    }
    void syncThreads() {ecolab::syncThreads();}
  };
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
