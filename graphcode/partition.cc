/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graphcode.h"
#include <utility>
#include <map>
#include "classdesc_epilogue.h"
#ifdef ECOLAB_LIB
#include "ecolab_epilogue.h"
#endif

namespace GRAPHCODE_NS
{
  using std::pair;
  using std::map;

  template <class T>
  struct array /* simple array for ParMETIS use */
  {
    T *v;
    array(unsigned n) {v=new T[n];}
    ~array() {delete [] v;}
    operator T*() {return v;}
    T& operator[](unsigned i) {return v[i];}
  };

#ifdef MPI_SUPPORT
  void check_add_reverse_edge(vector<vector<unsigned> >& nbrs, MPIbuf& b)
    {
      pair<unsigned,unsigned> edge;
      while (b.pos()<b.size())
	{
	  b>>edge;
	  vector<unsigned>::iterator begin=nbrs[edge.first].begin(),
	    end=nbrs[edge.first].end();
	  if (find(begin,end,edge.second)==end) /* edge not found, insert */
	    nbrs[edge.first].push_back(edge.second);
	}
    }
#endif

  void Graph::Partition_Objects()
  {
#if defined(MPI_SUPPORT) && defined(PARMETIS)
    if (nprocs()==1) return;
    Prepare_Neighbours(); /* used for computing edgeweights */
    unsigned i, j, nedges, nvertices=objects.size();

    /* ParMETIS needs vertices to be labelled contiguously on each processor */
    map<GraphID_t,unsigned int> pmap; 			
    vector<idxtype> counts(nprocs()+1);
    for (i=0; i<=nprocs(); i++) 
      counts[i]=0;

    /* label each pin sequentially within processor */
    omap::iterator pi;
    for (pi=objects.begin(); pi!=objects.end(); pi++) 
      pmap[pi->ID]=counts[pi->proc+1]++;

    /* counts becomes running sum of counts of local pins */
    for (i=1; i<nprocs(); i++) counts[i+1]+=counts[i];

    /* add offset for each processor to map */
    for (pi=objects.begin(); pi!=objects.end(); pi++)  
      pmap[pi->ID]+=counts[pi->proc];

    /* construct a set of edges connected to each local vertex */
    vector<vector<unsigned> > nbrs(nvertices);
    iterator p;
    {
      MPIbuf_array edgedist(nprocs());    
      for (p=begin(); p!=end(); p++)
	for (iterator n=(*p)->begin(); n!=(*p)->end(); n++)
	  {
	    if (n->ID==p->ID) continue; /* ignore self-links */
	    nbrs[pmap[p->ID]].push_back(pmap[n->ID]);
	    edgedist[n->proc] << pair<unsigned,unsigned>(pmap[n->ID],pmap[p->ID]);
	  }

      /* Ensure reverse edge is in graph (Metis requires graphs to be undirected */
      tag++;
      for (i=0; i<nprocs(); i++) if (i!=myid()) edgedist[i].isend(i,tag);
      /* process local list first */
      check_add_reverse_edge(nbrs,edgedist[myid()]);
      /* now get them from remote processors */
      for (i=0; i<nprocs()-1; i++)
	check_add_reverse_edge(nbrs,MPIbuf().get(MPI_ANY_SOURCE,tag));
    }

    /* compute number of edges connected to vertices local to this processor */
    nedges=0;
    for (int i=counts[myid()]; i<counts[myid()+1]; i++) 
      nedges+=nbrs[i].size();

    vector<idxtype> offsets(size()+1);
    vector<idxtype> edges(nedges);
    vector<idxtype> partitioning(size());

    /* fill adjacency arrays suitable for call to METIS */
    offsets[0]=0; 
    nedges=0;
    for (int i=counts[myid()]; i<counts[myid()+1]; i++)
      {
	for (vector<unsigned>::iterator j=nbrs[i].begin(); j!=nbrs[i].end(); j++)
	    edges[nedges++]=*j;
	offsets[i-counts[myid()]+1]=nedges;
      }

    int weightflag=3, numflag=0, nparts=nprocs(), edgecut, ncon=1;
    vector<float> tpwgts(nparts);
    vector<idxtype> vwgts(size()), ewgts(nedges);
    for (p=begin(), i=0; p!=end(); p++, i++) vwgts[i]=(*p)->weight();
    /* reverse pmap */
    vector<GraphID_t> rpmap(nvertices); 			
    for (pi=objects.begin(); pi!=objects.end(); pi++) 
      rpmap[pmap[pi->ID]]=pi->ID;
    for (p=begin(), i=1, j=0; p!=end(); p++, i++) 
      for (; j<unsigned(offsets[i]); j++)
	ewgts[j]=(*p)->edgeweight(objects[rpmap[(unsigned)edges[j]]]); 
    for (i=0; i<unsigned(nparts); i++) tpwgts[i]=1.0/nparts;
    float ubvec[]={1.05};
    int options[]={0,0,0,0,0}; /* for production */
    //int options[]={1,0xFF,15,0,0};  /* for debugging */
    MPI_Comm comm=MPI_COMM_WORLD;
    ParMETIS_V3_PartKway(&counts[0],&offsets[0],&edges[0],&vwgts[0],&ewgts[0],
			&weightflag,&numflag,&ncon,&nparts,&tpwgts[0],ubvec,options,
			&edgecut,&partitioning[0],&comm);

    rec_req.clear(); /* destroy record of previous communication patterns */

#if 0
    /* this simple minded code updates processor locations, pulls all
       data to the master, then redistributes - replaces the more
       complex code after the #if 0, which doesn't seem to work ... */
    for (p=begin(); p!=end(); p++)
      p->proc=partitioning[pmap[p->ID]-counts[myid()]];
      
    gather();
    Distribute_Objects();
    return;
#endif

    /* prepare pins to be sent to remote processors */
    MPIbuf_array sendbuf(nprocs());
    MPIbuf pin_migrate_list;
    for (p=begin(); p!=end(); p++)
      {
        p->proc=partitioning[pmap[p->ID]-counts[myid()]];
	if (p->proc!=myid()) 
	  {
	    sendbuf[p->proc]<<p->ID<<*p;
	    p->nullify();
	    pin_migrate_list << p->proc << p->ID;
	  }
      }

    /* clear list of local pins, then add back those remining locally */
    //    for (clear(), i=0; i<stayput.size(); i++) push_back(stayput[i]);

    /* send pins to remote processors */
    tag++;

    for (i=0; i<nprocs(); i++)
      {
	if (i==myid()) continue;
	sendbuf[i].isend(i,tag);
      }

    /* receive pins from remote processors */
    for (i=0; i<nprocs()-1; i++)
      {
	MPIbuf b; b.get(MPI_ANY_SOURCE,tag);
	objref pin; GraphID_t index;
	while (b.pos()<b.size()) 
	  {
	    b >> index;
	    b >> objects[index];
	  }
      }

 
   /* update proc records on all prcoessors */
    pin_migrate_list.gather(0);
    pin_migrate_list.bcast(0);
    while (pin_migrate_list.pos() < pin_migrate_list.size())
      {
	GraphID_t index; unsigned proc;
	pin_migrate_list >> proc >> index;
	objects[index].proc=proc;
      }
    rebuild_local_list();
#endif
  }
}
