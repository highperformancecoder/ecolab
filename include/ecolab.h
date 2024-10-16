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

#ifdef SYCL_LANGUAGE_VERSION
#include <sycl/sycl.hpp>
#endif

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
#endif
  
  template <class Cell> struct EcolabGraph: public graphcode::Graph<Cell>
  {
    /// apply a functional to all local cells of this processor in parallel
    /// @param f 
    template <class F>
    void forAll(F f) {
#ifdef SYCL_LANGUAGE_VERSION
      syclQ().submit([&](sycl::handler& h) {
        h.parallel_for(this->size(), [=,this](auto i) {
          f(*(*this)[i]->template as<Cell>());
        });
      });
#else
      for (auto& i: *this) f(*i->template as<Cell>());
#endif
    }
  };
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

#include "ecolab.cd"
#endif  /* ECOLAB_H */
