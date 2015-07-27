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
  void first(TCL_args args) { ///< create a graph with n nodes and l links
    unsigned n=args, l=args;
    g=BitRep(n,l); dg=BiDirectionalBitRep(n,l);}
  void next(TCL_args) { ///< create a graph with n nodes and l links
    BitRep b(g); b.next_perm(); g=b;
    BiDirectionalBitRep db(dg); db.next_perm(); dg=db;
  }
  array<unsigned> degrees() {
    Degrees d=::degrees(g);
    return array<unsigned>(d.in_degree+d.out_degree);
  }
  urand uni;
  void randGraph(TCL_args args) {ErdosRenyi_generate(g,args[0],args[1],uni,uni);
  }
  void prefAttach(TCL_args args) {PreferentialAttach_generate(g,args,uni);}
};
