/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/*
  TCL graph types - needs to be compiled at lower optimisation level,
  and included as a module
*/
#include "graph.h"
#ifdef IGRAPH
#include "igraph.h"
#endif
#include "ecolab_epilogue.h"

int ecolab_tclgraph_link;

namespace 
{
  typedef ecolab::ConcreteGraph<ecolab::DiGraph> DiGraph;
  TCLPOLYTYPE(DiGraph,ecolab::Graph);
  typedef ecolab::ConcreteGraph<ecolab::BiDirectionalGraph> BiDirectionalGraph;
  TCLPOLYTYPE(BiDirectionalGraph,ecolab::Graph);
#ifdef IGRAPH
  typedef ecolab::ConcreteGraph<ecolab::IGraph> IGraph;  
  TCLPOLYTYPE(IGraph,ecolab::Graph);
  // a directed IGraph
  struct diIGraph: public ecolab::IGraph
  {
    diIGraph(int nodes=0): ecolab::IGraph(nodes, true) {}
  };
  typedef ecolab::ConcreteGraph<diIGraph> DiIGraph;  
  TCLPOLYTYPE(DiIGraph,ecolab::Graph);

#endif
}
