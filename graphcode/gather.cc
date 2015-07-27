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
  void Graph::gather()
  {
#ifdef MPI_SUPPORT
    Ptrlist::iterator p;
    MPIbuf b; 
    if (myid()>0) 
      for (p=begin(); p!=end(); p++) 
	{
	  b<<p->ID<<*p;  /* this sends ID twice ! */
	  assert(!p->nullref());
	}
    b.gather(0);
    if (myid()==0)
      {
	while (b.pos()<b.size())
	  {
	    GraphID_t id; b>>id;
	    b>>objects[id];
	  }
      }
#endif
  }
}
