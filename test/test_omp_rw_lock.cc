/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "omp_rw_lock.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
using namespace ecolab;

int main()
{
#ifdef _OPENMP

  volatile bool running = true;
  volatile unsigned read=0, write=0;
  RWlock lock;
#pragma omp parallel sections num_threads(3)
  {
    // Thread 0 check exclusion of read & write locks
#pragma omp section
    while (running)
      assert(!(read && write));

    // Thread 1 attempt read locks
#pragma omp section
    {
      {
        read_lock r(lock);
#pragma omp atomic
        read |= 1;
        sleep(1); //wait for thread 2 to attempt write lock
#pragma omp atomic
        read &= ~1;
      }
      sleep(1);
      {
        read_lock r(lock);
#pragma omp atomic
        read |= 1;
        sleep(1);
#pragma omp atomic
        read &= ~1;
      }
      while (running);
    }

    // Thread 2
#pragma omp section
    {
      {
        read_lock r(lock);
#pragma omp atomic
        read |= 2;
        while (!(read&1)); //wait until Thread 1 sets readlock
        assert(read==3); //check both read locks active
#pragma omp atomic
        read &= ~2;
        write_lock w(lock);
#pragma omp atomic
        write |= 2;
        sleep(2);
        write &= ~2;
      }
      running=false;
    }
  }
#endif

  return 0;
}
   
