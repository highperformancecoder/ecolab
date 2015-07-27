/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/*
  This application generates all graph bit representations with n
  nodes and l links. It then sorts these into equivalence sets, and
  counts the size of each equivalence set.  It checks this against the
  lnomega function. This serves to test the assumptions of how Nauty
  works, and also as a exemplar of how to search all graphs of a
  certain size.
*/

#include "netcomplexity.h"
#include "nauty.h"
#include "ecolab_epilogue.h"
#include <sstream>
using namespace std;
using namespace ecolab;

typedef map<NautyRep,double> archetype;

bool nearly_equal(double x,double y)
{
  return fabs(x-y) < 1e-10*(fabs(x)+fabs(y));
}

Tcl_Interp *interp;

template <class B>
void test_iterator()
{
  B b(10);
  unsigned cnt1=0, cnt2=0;
  for (unsigned i=0; i<b.nodes(); i+=2)
    for (unsigned j=0; j<b.nodes(); j+=2)
      if (i!=j)
        {
          b.push_back(Edge(i,j));
          cnt1++;
        }
  for (typename B::const_iterator i=b.begin(); i!=b.end(); ++cnt2, ++i)
    assert(i->source()%2==0 && i->target()%2==0);
  assert(cnt1==cnt2);
}

template <class G>
void do_all_perm(archetype& arch, G& g)
{
  for (bool run=true; run; run=g.next_perm() )
    {
      NautyRep nauty_canon=canonicalise(g);
      arch[nauty_canon]++;
      /** ensure new algorithm produces a canonical graph that is equivalent
          to nauty's */
      BitRep canon(g), nCanon;
      assert(g.nodes()==canon.nodes() && g.links()==canon.links());
      canonical(canon,g);
      assert(DiGraph(g).nodes()==canon.nodes() && g.links()==canon.links());
      canonical(nCanon,nauty_canon);
      assert(DiGraph(g).nodes()==nCanon.nodes() && g.links()==nCanon.links());
      assert(canon==nCanon);
      assert(nauty_canon==canonicalise(canon));
      assert(nauty_canon==canonicalise(nCanon));
    }
}

int main()
{
  const int nodes=5, links=5;

  //test setrange
  {
    bitvect b(256);
    // test a single word fill
    b.setrange(5,20,true);
    for (unsigned i=0; i<5; ++i) assert(b[i]==false);
    for (unsigned i=5; i<20; ++i) assert(b[i]==true);
    for (unsigned i=20; i<b.size(); ++i) assert(b[i]==false);
  
    //test to just on boundary
    b.setrange(5,WORDSIZE,true);
    for (unsigned i=0; i<5; ++i) assert(b[i]==false);
    for (unsigned i=5; i<WORDSIZE; ++i) 
      assert(b[i]==true);
    for (unsigned i=WORDSIZE; i<b.size(); ++i) assert(b[i]==false);
    
    b.setrange(5,2*WORDSIZE,true);
    for (unsigned i=0; i<5; ++i) assert(b[i]==false);
    for (unsigned i=5; i<2*WORDSIZE; ++i) assert(b[i]==true);
    for (unsigned i=2*WORDSIZE; i<b.size(); ++i) assert(b[i]==false);
    
    b.setrange(5,b.size(),true);
    for (unsigned i=0; i<5; ++i) assert(b[i]==false);
    for (unsigned i=5; i<b.size(); ++i) assert(b[i]==true);
  }    

  //test popcnt
  for (unsigned i=0; i<256; ++i)
    {
      bitvect b(i,true);
      assert(b.popcnt()==i);
      if (i>5)
        {
          assert(b.popcnt(0,5)==5);
          assert(b.popcnt(5)==i-5);
        }
    }

  //test logic ops
  {
    bitvect x1(128), x2(128);
    for (unsigned i=0; i<x1.size(); i+=2) x1[i]=true;
    for (unsigned i=1; i<x1.size(); i+=2) x2[i]=true;
    assert((x1|x2).popcnt()==x1.size());
    assert((x1|x1)==x1);
    assert((x1&x2).popcnt()==0);
    assert((x1&x1)==x1);
    assert((x1^x2).popcnt()==x1.size());
    assert((x1^x1).popcnt()==0);
    assert(~x1==x2);
  }

  //test iterators work as expected
  {
    test_iterator<BitRep>();
    test_iterator<BiDirectionalBitRep>();
    test_iterator<NautyRep>();
    test_iterator<ConcreteGraph<BitRep> >();
    test_iterator<ConcreteGraph<BiDirectionalBitRep> >();
    test_iterator<ConcreteGraph<NautyRep> >();
  }

  // do an exhaustive search of the bidirectional bitrep graphs
  {
    BiDirectionalBitRep g(nodes,links);
    archetype arch;
    do_all_perm(arch,g);

    int suma=0;
    for (archetype::const_iterator i=arch.begin(); i!=arch.end(); ++i)
    {
      assert(nearly_equal(log(i->second),i->first.lnomega()));
      BitRep canon;
      assert(nearly_equal(log(i->second),canonical(canon,i->first)));
      suma+=i->second;
    }
    assert(nearly_equal(log(suma),lnCombine(nodes*(nodes-1)/2,links)));
  }

  // do an exhaustive search of the directed bitrep graphs
  {
    BitRep g(nodes,links);
    archetype arch;
    do_all_perm(arch,g);

    int suma=0;
    for (archetype::const_iterator i=arch.begin(); i!=arch.end(); ++i)
    {
      assert(nearly_equal(log(i->second),i->first.lnomega()));
      BitRep canon;
      assert(nearly_equal(log(i->second),canonical(canon,i->first)));
      suma+=i->second;
    }
    assert(nearly_equal(log(suma),lnCombine(nodes*(nodes-1),links)));
  }
}
