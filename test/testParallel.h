#include "ecolab.h"
#include <set>
struct Test
{
  std::set<unsigned> runTest()
  {
    std::set<unsigned> procs;
#ifdef MPI_SUPPORT
    classdesc::MPIbuf buf;
    buf<<ecolab::myid();
    buf.gather(0);
    if (ecolab::myid()==0)
      {
        while (buf.pos()<buf.size())
          {
            unsigned p; buf>>p;
            procs.insert(p);
          }
        assert(procs.size()==ecolab::nprocs());
      }
#else
    procs.insert(0);
#endif
    return procs;
  }
};
