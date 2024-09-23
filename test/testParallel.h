#include "ecolab.h"
struct Test
{
  void runTest()
  {
#ifdef MPI_SUPPORT
    MPIBuf()<<ecolab::myid()<<send(0);
#endif
  }
};
