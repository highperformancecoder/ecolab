/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifdef _OPENMP
#include <omp.h>
#endif
#include <algorithm>
#include <vector>
#include "nauty.h"

// TODO: this code doesn't seem to be used, but cppcheck flags an
// uninitialised variable


extern "C" int findFirstOf(const set* x,unsigned len)
{
  //heuristics that work well on CoreII quad
#ifdef _OPENMP
  if (len<=1024 || omp_get_max_threads()==1)
#endif

    { // single processor implementation
       for (unsigned j=0; j<len; ++j)
         if (x[j]) return j;
       return len;
    }

#ifdef _OPENMP
  else 
    {
      unsigned min_loc=len;
      // we need to block this algorithm to avoid thrashing the cache
      unsigned int log2blk=11, blk;
      if (len<=2048) log2blk=8; 
      else if (len<=4096) log2blk=9;
      else if (len<=8192) log2blk=10;
      blk=1<<log2blk;

#pragma omp parallel
      {
        unsigned offs=omp_get_thread_num()*blk;
        unsigned stride=omp_get_num_threads()*blk;
        for (unsigned i=offs; i<min_loc; i+=stride)
          {
            unsigned maxj=std::min(i+blk,min_loc);
            for (unsigned j=i; j<maxj; ++j)
              if (x[j])
#pragma omp critical
                if (j<min_loc)
                  min_loc=j;
          }
      }
      return min_loc;
    }
#endif
}

