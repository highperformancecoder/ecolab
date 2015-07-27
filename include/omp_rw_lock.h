/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
   \brief A read/write lock pattern for OpenMP
*/

#ifndef OMP_RW_LOCK_H
#define OMP_RW_LOCK_H

#ifdef _OPENMP
#include <omp.h>


namespace ecolab
{
  ///   A read/write lock pattern for OpenMP
  class RWlock
  {
    /** 
        stores which threads are read locked. Implies a maximum of 32
        threads are supported on 32 bit machines ad 64 threads on 64
        bit machines.
    */
    volatile unsigned long read_mask; 
    omp_lock_t write_lock;
  public:
    RWlock(): read_mask(0) {omp_init_lock(&write_lock);}
    ~RWlock() {omp_destroy_lock(&write_lock);}

    void lock_for_read() {
      omp_set_lock(&write_lock);
#pragma omp atomic
      read_mask |= (1 << omp_get_thread_num());
      omp_unset_lock(&write_lock);
    }

    void unlock_for_read() {
#pragma omp atomic
      read_mask &= ~(1 << omp_get_thread_num());
    }

    void lock_for_write() {
      unsigned long read_set = read_mask & (1 << omp_get_thread_num());
      unlock_for_read();
      omp_set_lock(&write_lock);
      //wait for read locks on other threads to be relinquished
      while (read_mask);
#pragma omp atomic
      read_mask |= read_set; //reestablish previous read lock status.
    }

    void unlock_for_write() {
      omp_unset_lock(&write_lock);
    }    

  };

  /// apply a read lock to the current scope (prevents write lock from
  /// being taken)
  class read_lock
  {
    RWlock& lock;
  public:
    read_lock(RWlock& lock): lock(lock) {lock.lock_for_read();}
    ~read_lock() {lock.unlock_for_read();}
  };

  /// apply a write lock (exclusive access) to the current scope
  class write_lock
  {
    RWlock& lock;
  public:
    write_lock(RWlock& lock): lock(lock) {lock.lock_for_write();}
    ~write_lock() {lock.unlock_for_write();}
  };
}
#else //!_OPENMP
namespace ecolab
{
  class RWlock {};
  struct read_lock
  {
    read_lock(RWlock& lock) {}
  };

  struct write_lock
  {
    write_lock(RWlock& lock) {}
  };

} 
#endif
#endif
