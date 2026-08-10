/*
  @copyright Russell Standish 2026
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef DEVICE_ALLOCATOR_H
#define DEVICE_ALLOCATOR_H
#include "sycl.h"
#include "graphcode.h"

namespace ecolab
{
  /// minimum allocation size = 2^minOrder, maximum object size = 2^(maxOrder-1)
  constexpr unsigned minOrder=4, maxOrder=20;
  /// memory allocated to each order, total memory allocated on device=poolSize*(maxOrder-minOrder)
  constexpr unsigned poolSize=32*1024*1024;

  /// size of work groups when running on GPU
  extern unsigned workGroupSize;

  struct FatalErrorFlag
  {
    bool flag;
  };
  
  inline __attribute__((noinline)) bool& fatalErrorFlag() {
#ifdef  SYCL_LANGUAGE_VERSION
    return sycl::ext::oneapi::group_local_memory<FatalErrorFlag>(syclGroup(),false)->flag;
#else
    static bool flag;
    return flag;
#endif
  }
  
  // Bounded MPMC circular buffer queue for SYCL using per-slot sequence numbers.
  // dequeue() returns ~0U when queue appears empty (non-blocking empty signal).
  template <unsigned size>
  class Stack
  {
    static_assert((size&(size-1))==0,"size must be power of two");
    constexpr static unsigned mask=size-1;

    unsigned slots[size];
    unsigned top=size; //empty stack, stack grows down

    using Atomic=sycl::atomic_ref<unsigned,sycl::memory_order::acq_rel,sycl::memory_scope::device>;
    CLASSDESC_ACCESS(Stack);
  public:
    void init() {
      top=0; // full stack
      for (unsigned i=syclItem().get_global_linear_id(); i<size;
           i+=syclItem().get_global_range().size()) 
          slots[i]=i;
    }
    
    void push(unsigned x)
    {
      slots[--Atomic(top)]=x;
    }

    unsigned pop()
    {
      Atomic t(top);
      unsigned p=t++;
      if (p>=size) {t=size; return ~0;} // stack empty
      return slots[p];
    }

    // move contents of \a x onto this. Not threadsafe, call from host
    void appendAndDiscard(Stack& x) {
      top-=size-x.top;
      memcpy(slots+top, x.slots+x.top, (size-x.top)*sizeof(slots[0]));
      x.top=size;
    }
  };

  template <unsigned order> class DeviceAllocator;
  /// empty allocator to terminate template recursion
  template <> class DeviceAllocator<ecolab::maxOrder> {
  public:
    void* allocate(size_t sz) {
      if (groupLeader())
        sycl::ext::oneapi::experimental::printf("failed to allocate %zu bytes\n",sz);
#ifdef __SYCL_DEVICE_ONLY__
      fatalErrorFlag()=true;
#else
      throw std::bad_alloc();
#endif
      return nullptr;
    }
    void deallocate(void* p, size_t) {sycl::ext::oneapi::experimental::printf("%p leaked on device\n",p);}
    void init() {}
    void recycleDiscardPile() {}
  };

  template <unsigned order=minOrder> class DeviceAllocator
  {
    constexpr static unsigned pageSize=1<<order;
    constexpr static unsigned numPages=poolSize/pageSize;
    Stack<numPages> queue;
    Stack<numPages> discard; // discard pile
    char memory[poolSize];
    DeviceAllocator<order+2> nextAllocator; // next size up allocator
    CLASSDESC_ACCESS(DeviceAllocator);
  public:
    void init() {
      auto chunkOWork=syclQ().get_device().
        get_info<sycl::info::device::max_compute_units>()*workGroupSize;
      syclQ().parallel_for(std::min(chunkOWork,unsigned(numPages)),
                           [this](size_t) {queue.init();});
      nextAllocator.init();
    }
    void recycleDiscardPile() {
      queue.appendAndDiscard(discard);
      nextAllocator.recycleDiscardPile();
    }
    // all members of group get the same pointer
    void* allocate(size_t size) {
      if (size==0) return nullptr;
      if (size<=pageSize) {
        unsigned offs=~0U;
        if (localThreadId()==0) offs=queue.pop();
#ifdef __SYCL_DEVICE_ONLY__
        offs=sycl::group_broadcast(syclGroup(),offs,0);
#endif
        if (offs!=~0U)
          return memory+(offs<<order);
      }
      return nextAllocator.allocate(size);
    }
    void deallocate(void* p, size_t size) {
      if (!p) return;
      if (p>=memory && p<memory+poolSize) {
#ifdef __SYCL_DEVICE_ONLY__
        if (groupLeader())
          // push onto discard pile to avoid race condition
          discard.push((reinterpret_cast<char*>(p)-memory)>>order);
#else
        // on host, we can push back onto stack. Note this is not
        // threadsafe, so not to be used with OpenMP.
        queue.push((reinterpret_cast<char*>(p)-memory)>>order);
#endif
        return;
      }
      nextAllocator.deallocate(p,size);
    }
    
    bool inAllocator(void* p) const {return p>=this && p<this+1;}
  };
  
  inline DeviceAllocator<>& deviceAllocator() {
    static DeviceType<DeviceAllocator<>> deviceAllocator;
    static int dummy=
      (deviceAllocator->init(), syclQ().wait_and_throw(), 0);      
    return *deviceAllocator;
  }

  /// Allocator wrapping the DeviceAllocator singleton
  template <class T>
  struct GlobalDeviceAllocator
  {
    using value_type=T;
    using pointer=T*;
    using reference=T&;
    using difference_type=std::ptrdiff_t;
    using propagate_on_container_move_assignment=std::true_type;

    DeviceAllocator<>* allocator;
    
#ifdef __SYCL_DEVICE_ONLY__
    GlobalDeviceAllocator(): allocator(nullptr) {} // = delete;
#else
    GlobalDeviceAllocator() // note: default constructor must be called on host
    {allocator=&deviceAllocator();}
#endif
    template <class U>
    GlobalDeviceAllocator(const GlobalDeviceAllocator<U>& other):
      allocator(other.allocator) {}
    
    T* allocate(size_t n)
    {return reinterpret_cast<T*>(allocator->allocate(n*sizeof(T)));}
    void deallocate(T* p, size_t n){allocator->deallocate(p,n*sizeof(T));}
    template<class U> struct rebind {using other=GlobalDeviceAllocator<U>;};
    // allocator is stateless
    bool operator==(const GlobalDeviceAllocator&) const {return true;}
  };

  template <class T>
  struct HostSharedAllocator: public graphcode::Allocator<T>
  {
    HostSharedAllocator(): graphcode::Allocator<T>(syclQ(), sycl::usm::alloc::shared) {}
    template<class U> struct rebind {using other=HostSharedAllocator<U>;};
    // allocator is stateless
    bool operator==(const HostSharedAllocator&) const {return true;}
  };

  constexpr static unsigned LocalAllocatorSize=8*1024; // 32KiB = half typical local storage

  struct LocalAllocatorBuffer
  {
    unsigned next;
    char buffer[LocalAllocatorSize];
  };

  /*
    group_local_memory is a weird beast. The address returned is tied to the line of code in which it instantiated, so we need to specify noinline to prevent it from being inlined, and inline to ensure single definition
  */
  inline __attribute__((noinline)) LocalAllocatorBuffer& localAllocatorBuffer()
   {return *sycl::ext::oneapi::group_local_memory_for_overwrite<LocalAllocatorBuffer>(syclGroup());}

  
#ifdef __SYCL_DEVICE_ONLY__
  
  /**
     A Local allocator allocates memory from device local memory,
     which is shared between threads of a work group, and has the same
     lifetime as the kernel.
     LocalAllocatorT so we can expose LocalAllocator as a template alias on both host and device branches
  */
  template <class T>
  class LocalAllocatorT
  {
  public:
    using value_type=T;
    using pointer=T*;
    using reference=T&;
    using difference_type=std::ptrdiff_t;
    using propagate_on_container_move_assignment=std::true_type;

    // no need for destructor, as Impl has nothing to tear down
    T* allocate(size_t n) {
      auto& b=localAllocatorBuffer();
      unsigned offs=b.next;
      if (offs+n*sizeof(T)>LocalAllocatorSize)
        {
          fatalErrorFlag()=true;
          return nullptr;
        }
      sycl::group_barrier(syclGroup());
      if (syclGroup().leader()) b.next+=n*sizeof(T);
      sycl::group_barrier(syclGroup());
      char* alloc=b.buffer+offs;
      return reinterpret_cast<T*>(alloc);
    }
    void deallocate(T*p,size_t) {} // cleaned up when group exits
    template<class U> struct rebind {using other=LocalAllocatorT<U>;};
    // allocator is stateless
    bool operator==(const LocalAllocatorT&) const {return true;}
  };
  template <class T> using LocalAllocator=LocalAllocatorT<T>;
#else
  template <class T> class LocalAllocatorT {};
  template <class T> using LocalAllocator=std::allocator<T>;
#endif
   
}

#include "DeviceAllocator.cd"
#endif
