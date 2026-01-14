/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graph.h"
#include "tcl++.h"
#include "ecolab_epilogue.h"

#include <boost/graph/edge_list.hpp>
using namespace std;
using namespace boost;
using namespace ecolab;

int main()
{
  ConcreteGraph<DiGraph> g;
  urand uni;

  ErdosRenyi_generate(g,100,500,uni);

  typedef boost::edge_list<ConcreteGraph<DiGraph>::const_iterator> BoostGraph;
  BoostGraph bg(g.begin(),g.end());
  typedef BoostGraph::edge_iterator Ei;
  assert(num_edges(bg)==g.links());
  std::pair<Ei,Ei> edge_range=edges(bg);
  unsigned i=0;
  for (Ei e=edge_range.first; e!=edge_range.second; ++i, ++e)
    assert(g.contains(Edge(source(*e,bg),target(*e,bg))));
  assert(i==g.links());

  // many BGL algorithms will write into an output iterator. We can
  // also std::copy from a Boost Graph object
  ConcreteGraph<DiGraph> g1;
  copy(edge_range.first, edge_range.second, g1.back_inserter(bg));
  assert(g1==g);

}
