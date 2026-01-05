/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

using namespace ecolab;
using array_ns::array;

struct Testgraph
{
  ConcreteGraph<DiGraph> g;
  ConcreteGraph<BiDirectionalGraph> dg;
  void first(unsigned n, unsigned l) { ///< create a graph with n nodes and l links
    g.asg(BitRep(n,l)); dg.asg(BiDirectionalBitRep(n,l));}
  void next() { ///< create a graph with n nodes and l links
    BitRep b(g); b.next_perm(); g.asg(b);
    BiDirectionalBitRep db(dg); db.next_perm(); dg.asg(db);
  }
  array<unsigned> degrees() {
    Degrees d=::degrees(g);
    return array<unsigned>(d.in_degree+d.out_degree);
  }
  urand uni;
  void randGraph(unsigned nodes, unsigned links) {ErdosRenyi_generate(g,nodes,links,uni,uni);
  }
  void prefAttach(unsigned nodes) {PreferentialAttach_generate(g,nodes,uni);}
};
