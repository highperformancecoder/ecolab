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

  // Lock free circular buffer queue for SYCL
  template <unsigned size>
  class Queue
  {
    static_assert((size&(size-1))==0,"size must be power of two");
    constexpr static unsigned mask=size-1;
    unsigned queue[size];
    unsigned head=0, tail=0;
    using Atomic=sycl::atomic_ref<unsigned,sycl::memory_order::relaxed,sycl::memory_scope::system>;

  public:
    void enqueue(unsigned x)
    {
      Atomic h(head);
      queue[h++ & mask]=x;
    }
    unsigned dequeue()
    {
      Atomic h(head), t(tail);
      if (h==t) return ~0U; // signal buffer empty, don't wait
      return queue[t++ & mask];
    }
  };

  template <unsigned order> class DeviceAllocator;
  /// empty allocator to terminate template recursion
  template <> class DeviceAllocator<maxOrder> {
  public:
    void init(sycl::queue& q) {}
    void* allocate(size_t sz) {return nullptr;}
    void deallocate(void* p, size_t) {sycl::ext::oneapi::experimental::printf("%x leaked on device\n",p);}
  };

  template <unsigned order=minOrder> class DeviceAllocator
  {
    constexpr static unsigned pageSize=1<<order;
    constexpr static unsigned numPages=poolSize/pageSize;
    Queue<numPages> queue;
    char memory[poolSize];
    DeviceAllocator<order+1> nextAllocator; // next size up allocator
  public:
    void init(sycl::queue& q) {
      q.parallel_for(numPages, [this](auto i) {
        queue.enqueue(i<<order);
      });
      nextAllocator.init(q);
    }
    void* allocate(size_t size) {
      if (size==0) return nullptr;
      if (size<=pageSize) {
        auto offs=queue.dequeue();
        if (offs!=~0U)
          return memory+offs;
      }
      return nextAllocator.allocate(size);
    }
    void deallocate(void* p, size_t size) {
      if (!p) return;
      if (p>=memory && p<memory+numPages*pageSize) {
        queue.enqueue((reinterpret_cast<char*>(p)-memory)>>order);
        return;
      }
      nextAllocator.deallocate(p,size);
    }
    
    bool inAllocator(void* p) const {return p>=this && p<this+1;}
  };
  
  inline DeviceAllocator<>& deviceAllocator() {
    static DeviceType<DeviceAllocator<>> deviceAllocator;
    static int initialised=(deviceAllocator->init(syclQ()),0);
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

    DeviceAllocator<>* allocator=&deviceAllocator();
    
    GlobalDeviceAllocator() = default; // note: default constructor must be called on host
    template <class U>
    GlobalDeviceAllocator(const GlobalDeviceAllocator<U>& other):
      allocator(other.allocator) {}
    
    T* allocate(size_t n)
    {return reinterpret_cast<T*>(allocator->allocate(n*sizeof(T)));}
    void deallocate(T* p, size_t n){allocator->deallocate(p,n*sizeof(T));}
    template<class U> struct rebind {using other=GlobalDeviceAllocator<U>;};
  };

  template <class T>
  struct HostSharedAllocator: public graphcode::Allocator<T>
  {
    HostSharedAllocator(): graphcode::Allocator<T>(syclQ(), sycl::usm::alloc::shared) {}
    template<class U> struct rebind {using other=HostSharedAllocator<U>;};
  };
    
}
#endif
