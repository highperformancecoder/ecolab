/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graphcode.h"
#include "classdesc_epilogue.h"
#ifdef ECOLAB_LIB
#include "ecolab_epilogue.h"
#endif

namespace GRAPHCODE_NS 
{
  void Graph::Prepare_Neighbours(bool cache_requests)
  {
#ifdef MPI_SUPPORT
    if (nprocs()==1) return;
    vector<vector<objref> > return_data(nprocs());
    
    if (!cache_requests || rec_req.size()!=nprocs())
      {
	rec_req.clear();
	rec_req.resize(nprocs());
	requests.clear();
	requests.resize(nprocs());
        vector<set<GraphID_t> > uniq_req(nprocs());
	/* build a list of ID requests to be sent to processors */
	for (iterator obj1=begin(); obj1!=end(); obj1++)
	  for (Ptrlist::iterator obj2=(*obj1)->begin(); obj2!=(*obj1)->end(); obj2++)
	      if (obj2->proc!=myid())
		uniq_req[obj2->proc].insert(obj2->ID);

	/* now send & receive requests */
	tag++;
	MPIbuf_array sendbuf(nprocs());
	for (unsigned proc=0; proc<nprocs(); proc++)
	  {
	    if (proc==myid()) continue;
	    sendbuf[proc] << uniq_req[proc] >> requests[proc];
            sendbuf[proc].isend(proc,tag);
	  }
	for (unsigned i=0; i<nprocs()-1; i++)
	  {
	    MPIbuf b; 
	    b.get(MPI_ANY_SOURCE,tag);
	    b >> rec_req[b.proc];
	  }
      }

    /* now service requests */
    tag++;
    MPIbuf_array sendbuf(nprocs());
    for (unsigned proc=0; proc<nprocs(); proc++)
      {
	if (proc==myid()) continue;
	unsigned i;
	for (i=0; i<rec_req[proc].size(); i++)
	  sendbuf[proc] << objects[rec_req[proc][i]];
	sendbuf[proc].isend(proc,tag);
      }
    for (unsigned p=0; p<nprocs()-1; p++)
      {
	MPIbuf b; b.get(MPI_ANY_SOURCE,tag);
	for (unsigned i=0; i<requests[b.proc].size(); i++) 
	  b>>objects[requests[b.proc][i]];
      }
#endif /* MPI_SUPPORT */
  }
}
