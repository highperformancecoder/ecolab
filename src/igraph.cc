/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifdef IGRAPH
#include "igraph.h"
#include "ecolab_epilogue.h"
#include <vector>

using namespace ecolab;
using namespace std;

namespace 
{
  void handler(const char* reason, const char* file, int line, int err)
  {
    IGRAPH_FINALLY_FREE();
    throw IGraphError(err);
  }
}

void ecolab::IGraph::addEdges(const Graph& g)
{
  vector<igraph_real_t> edges; edges.reserve(2*g.links());
  for (Graph::const_iterator e=g.begin(); e!=g.end(); ++e)
    {edges.push_back(e->source()); edges.push_back(e->target());}
  igraph_vector_t iv;
  igraph_vector_view(&iv,&edges[0],edges.size());
  igraph_add_edges(this,&iv,0);
}

// registers the error handler at class load time
int IGraph::errorhandler=(igraph_set_error_handler(handler), 0);
                          
#endif
