/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief Network complexity measures
*/
#ifndef NETCOMPLEXITY_H
#define NETCOMPLEXITY_H

#ifdef MPI_SUPPORT
#include <classdescMP.h>
using classdesc::MPIbuf;
#endif
#include <pack_base.h>

#include <ref.h>
#include <graph.h>

#include <ref.h>
#include <graph.h>

#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>
#include <iostream>
#include <stdio.h>

using std::vector;
using std::map;
using std::pair;
using std::insert_iterator;
using std::max_element;
using std::set_difference;
using std::ostream;
using std::cout;
using std::endl;
using std::max;

#include <math.h>
#include <assert.h>

/*
  bits needed from nauty.h, macros replaced with inline C++ functions 
*/

namespace ecolab
{
  // use WORDSIZE to select 32 or 64 bit sets, 16 bit sets are not to be supported.
#ifdef WORDSIZE
#define ECOLAB_WORDSIZE WORDSIZE
#else // 64 bit sets are default.
#define ECOLAB_WORDSIZE 64
#endif

#if ECOLAB_WORDSIZE==64
#if UINT_MAX>0xFFFFFFFF
  typedef unsigned setword;
#elif ULONG_MAX>  0xFFFFFFFF
  typedef unsigned long setword;
#else
  typedef unsigned long long setword;
#endif
#elif ECOLAB_WORDSIZE==32
  typedef unsigned int setword;
#else
#error "No integer format matches WORDSIZE"
#endif

  
#if ECOLAB_WORDSIZE==64
  const setword MSK64=0xFFFFFFFFFFFFFFFFULL;
  const setword MSK63C=0x7FFFFFFFFFFFFFFFULL;
  const setword ALLBITS=MSK64;
  inline unsigned SETWD(unsigned pos) {return pos>>6;}
  inline unsigned SETBT(unsigned pos) {return pos&0x3F;}
  inline setword BITMASK(unsigned x)  {return MSK63C >> x;}

  const setword bitt[] = {01000000000000000000000LL,0400000000000000000000LL,
                          0200000000000000000000LL,0100000000000000000000LL,
                          040000000000000000000LL,020000000000000000000LL,
                          010000000000000000000LL,04000000000000000000LL,
                          02000000000000000000LL,01000000000000000000LL,
                          0400000000000000000LL,0200000000000000000LL,
                          0100000000000000000LL,040000000000000000LL,
                          020000000000000000LL,010000000000000000LL,
                          04000000000000000LL,02000000000000000LL,
                          01000000000000000LL,0400000000000000LL,0200000000000000LL,
                          0100000000000000LL,040000000000000LL,020000000000000LL,
                          010000000000000LL,04000000000000LL,02000000000000LL,
                          01000000000000LL,0400000000000LL,0200000000000LL,
                          0100000000000LL,040000000000LL,020000000000LL,010000000000LL,
                          04000000000LL,02000000000LL,01000000000LL,0400000000LL,
                          0200000000LL,0100000000LL,040000000LL,020000000LL,
                          010000000LL,04000000LL,02000000LL,01000000LL,0400000LL,
                          0200000LL,0100000LL,040000LL,020000LL,010000LL,04000LL,
                          02000LL,01000LL,0400LL,0200LL,0100LL,040LL,020LL,010LL,
                          04LL,02LL,01LL};
#elif ECOLAB_WORDSIZE==32
  const setword MSK32=0xFFFFFFFFUL;
  const setword MSK31C=0x7FFFFFFFUL;
  const setword ALLBITS=MSK32;
  inline unsigned SETWD(unsigned pos) {return pos>>5;}
  inline unsigned SETBT(unsigned pos) {return pos&0x1F;}
  inline setword BITMASK(unsigned x)  {return MSK31C >> x;}

  const setword bitt[] = {020000000000,010000000000,04000000000,02000000000,
                          01000000000,0400000000,0200000000,0100000000,040000000,
                          020000000,010000000,04000000,02000000,01000000,0400000,
                          0200000,0100000,040000,020000,010000,04000,02000,01000,
                          0400,0200,0100,040,020,010,04,02,01};
#endif


  inline void ADDELEMENT(setword* setadd,unsigned pos)  
  {setadd[SETWD(pos)] |= bitt[SETBT(pos)];}

  inline void DELELEMENT(setword *setadd, unsigned pos) 
  {setadd[SETWD(pos)] &= ~bitt[SETBT(pos)];}

  inline void FLIPELEMENT(setword *setadd, unsigned pos) 
  {setadd[SETWD(pos)] ^= bitt[SETBT(pos)];}

  inline bool ISELEMENT(const setword *setadd, unsigned pos) 
  {return (setadd[SETWD(pos)] & bitt[SETBT(pos)]) != 0;}

  inline void EMPTYSET(setword *setadd, unsigned m) 
  {
    setword *es; 
    for (es = setadd+m; --es >= setadd;) *es=0;
  }

#include <pack_base.h>
#include <pack_stl.h>
#include <classdesc_access.h>
#ifdef MPI_SUPPORT
#include <classdescMP.h>
#endif

  /* iota seems to be missing? */
  template <class T>
  void iota(typename T::iterator p, const typename T::iterator& end, typename T::value_type val)
  {for (; p!=end; p++, val++) *p=val;}

  inline void iota(vector<unsigned>::iterator p, const vector<unsigned>::iterator& end, unsigned val)
  {for (; p!=end; p++, val++) *p=val;}

  inline void iota(unsigned* p, const unsigned * end, unsigned val)
  {for (; p!=end; p++, val++) *p=val;}


  /* wrap nauty's bitset class with bitvector syntax */
  class bitref
  {
    setword *addr;
    unsigned idx;
    CLASSDESC_ACCESS(bitref);
  public:
    bitref(setword *a, unsigned i): addr(a), idx(i) {}
    operator bool () const {return ISELEMENT(addr,idx);}
    bitref& operator=(bool x) {x? ADDELEMENT(addr,idx): DELELEMENT(addr,idx); return *this;}
    bitref& operator=(const bitref& x) {return (*this)=(bool)x;}
    bool operator==(const bitref x) const {return (bool)(*this)==(bool)x;}
    bool operator!=(const bitref x) const {return (bool)(*this)==(bool)x;}
    bool operator<(const bitref x)  const {return (bool)(*this)<(bool)x;}
  };

  class const_bitref
  {
    const setword *addr;
    unsigned idx;
    CLASSDESC_ACCESS(const_bitref);
  public:
    const_bitref(const setword *a, unsigned i): addr(a), idx(i) {}
    operator bool () const {return ISELEMENT(addr,idx);}
    bool operator==(const bitref x) const {return (bool)(*this)==(bool)x;}
    bool operator!=(const bitref x) const {return (bool)(*this)==(bool)x;}
    bool operator<(const bitref x)  const {return (bool)(*this)<(bool)x;}
  };

  class NautyRep;

  class bitvect
  {
    unsigned sz;
    std::vector<setword> data;

    unsigned nwords() const {return sz? div(sz-1)+1: 0;}
    unsigned nbytes() const {return nwords()*sizeof(setword);}
    unsigned div(unsigned i) const {return SETWD(i);}  /* i/WORDSIZE; */ 
    unsigned mod(unsigned i) const {return SETBT(i);} /* i%WORDSIZE; */
    unsigned mod(int i) const {return SETBT(i);} /* i%WORDSIZE; */
    void null_last_bits() {if (sz) data[nwords()-1]&=~BITMASK(mod(sz-1));}
    bool last_bits_null() const {
      return sz==0 || !(data[nwords()-1]&BITMASK(mod(sz-1)));
    }

    //needs to reach in to the data member
    friend void ecolab_nauty(const NautyRep&,NautyRep&,double&,bool,int);
    CLASSDESC_ACCESS(bitvect);
  public:

    bitvect(): sz(0), data(0) {}
    bitvect(const bitvect& x): sz(x.sz), data(x.data) {assert(last_bits_null());}
    bitvect(unsigned s, bool x=false): sz(s), data(nwords(),x?ALLBITS:0) {
      null_last_bits();}
    template <class T>
    bitvect(const vector<T>& x): sz(x.sz), data(nwords()) {
      for (unsigned i=0; i<sz; i++) (*this)[i]=x[i];
      assert(last_bits_null());
    }

    bitvect& operator|=(const bitvect& x)  {
      assert(x.data.size()==data.size());
      for (unsigned i=0; i<x.data.size(); ++i) data[i]|=x.data[i];
      assert(last_bits_null());
      return *this;
    }
    bitvect operator|(bitvect x) const {return x|=*this;}

    bitvect& operator&=(const bitvect& x)  {
      assert(x.data.size()==data.size());
      for (unsigned i=0; i<x.data.size(); ++i) data[i]&=x.data[i];
      assert(last_bits_null());
      return *this;
    }

    bitvect operator&(bitvect x) const {return x&=*this;}

    bitvect& operator^=(const bitvect& x)  {
      assert(x.data.size()==data.size());
      for (unsigned i=0; i<x.data.size(); ++i) data[i]^=x.data[i];
      null_last_bits();
      return *this;
    }
    bitvect operator^(bitvect x) const {return x^=*this;}

    bitvect operator~() const {
      bitvect r(*this);
      return r.flip();
    }
    
    bitvect& flip() {
      for (unsigned i=0; i<data.size(); ++i) data[i]=~data[i];
      null_last_bits();
      return *this;
    }

    bitvect operator>>(unsigned offs) const {
      bitvect r(sz+offs);
      for (unsigned i=0; i<nwords()-1; ++i) {
        r.data[div(i*ECOLAB_WORDSIZE+offs)+1] = data[i]>>mod(offs);
        r.data[div(i*ECOLAB_WORDSIZE+offs)]   = data[i]<<mod(-offs);
      }
      r.data[div((nwords()-1)*ECOLAB_WORDSIZE+offs)] = data[nwords()-1]<<mod(-offs);
      return r;
    }
    bitvect num(unsigned start,unsigned nbits) const;
    bool equal(const bitvect& y, unsigned len, unsigned offs) const;
    void setrange(unsigned start,unsigned end,bool value);
    bitref operator[] (unsigned i) {assert(i<sz); return bitref(data.data(),i);}
    const_bitref operator[] (unsigned i) const {assert(i<sz); return const_bitref(data.data(),i);}
    unsigned size() const {return sz;}
    /// increment the bitfield
    void operator++(int);
    ///next permutation sequence (nos of zeros & ones kept constant).
    /// returns true if there is a next permutation, false otherwise
    bool next_perm();
    unsigned popcnt(unsigned start=0, int end=-1 /* -1 == sz */) const;
    bool operator==(const bitvect& x) const 
    {
      bool r=x.sz==sz; 
      for (unsigned i=0; r&&i<nwords(); i++) r&=x.data[i]==data[i];
      return r;
    }
    bool operator!=(const bitvect& x)  const {return !((*this)==x);}
    bool operator<(const bitvect& x) const 
    {
      assert(last_bits_null());
      bool r=x.sz==sz; 
      unsigned i;
      for (i=0; i<nwords(); i++) if (!(r&=x.data[i]==data[i])) break;
      return !r && (sz<x.sz || (sz==x.sz && data[i]<x.data[i]));
    }
    void rand() { /* fill vector with random bits */
      for (unsigned i=0; i<nwords(); data[i++]=::rand()); 
      null_last_bits();
    }
  };


#ifdef _CLASSDESC
#pragma omit pack ecolab::bitref
#pragma omit unpack ecolab::bitref
#pragma omit pack ecolab::const_bitref
#pragma omit unpack ecolab::const_bitref
#endif

  /* return the number corresponding the nbit field pointed to by i */
  bitvect num(const bitvect& x, unsigned i,unsigned nbits);

  /*
    Common assignment between all of the *Rep types
  */
  template <class T, class U>
  void asgRep(T& x, const U& y)
  {
    if (x.nodes()!=y.nodes()) x=T(y.nodes());
    for (unsigned i=0; i<y.nodes(); ++i)
      for (unsigned j=0; j<y.nodes(); ++j)
        if (i!=j) x(i,j)=y(i,j);
  }

  /// convert between an EcoLab Graph type and a BitRep type
  template <class T, class Graph>
  void asgRepGraph(T& x, const Graph& y)
  {
    x=T(y.nodes());
    for (typename Graph::const_iterator i=y.begin(); i!=y.end(); ++i)
      x(i->source(),i->target())=true;
  }

  ///An iterator suitable for "duck-typing" with Graph
  template <class Rep>
  class Rep_const_iterator
  {
    Edge e;
    unsigned &i, &j;
    const Rep* rep;
  public:
    Rep_const_iterator(const Rep& rp, bool end): 
      e(0, 1), i(e.source()), j(e.target()), rep(&rp)
    {
      if (end||rep->nodes()<2) {i=rep->nodes(); j=0;} 
      else if (!(*rep)(i,j)) operator++();//initialise ref to point to first arc
    }
    Rep_const_iterator(const Rep_const_iterator& x): 
      i(e.source()), j(e.target()) {*this=x;}
    const Rep_const_iterator& operator=(const Rep_const_iterator& x)
    {e=x.e; rep=x.rep; return *this;}
    
    Edge operator*() const {return e;}
    const Edge* operator->() const {return &e;}
    void operator++() { //skip to next edge
      do {
        ++j;
        if (j==i) ++j;
        if (j>=rep->nodes())
          {
            j=0;
            ++i;
          }
      } while (i<rep->nodes() && !(*rep)(i,j));
    }
    bool operator==(const Rep_const_iterator& x) const {
      return e==x.e && rep==x.rep;
    }
    bool operator!=(const Rep_const_iterator& x) const {
      return !operator==(x);
    }
  };
      
  class BiDirectionalBitRep;
  class NautyRep;

  /**
     a directed graph with the linklist represented by a bitvector
  */
  class BitRep
  {
    bitvect linklist;
    unsigned nodecnt;
    CLASSDESC_ACCESS(BitRep);
    unsigned idx(unsigned i, unsigned j) const {
      assert(i!=j);
      return (j<i)? (nodecnt-1)*i+j: (nodecnt-1)*i+j-1;
    }
  public:
    typedef Rep_const_iterator<BitRep> const_iterator;
    const_iterator begin() const {return const_iterator(*this,false);}
    const_iterator end() const {return const_iterator(*this,true);}

    /// generate a node graph with l links at start of bit sequence (eg
    /// 11111000000....)
    BitRep(unsigned nodes=0, unsigned links=0): 
      linklist( nodes * (nodes-1), false), nodecnt(nodes) {
      assert(links<=linklist.size());
      linklist.setrange(0,links,true);
    }
    BitRep(const Graph& x): nodecnt(0) {asgRepGraph(*this,x);}
    BitRep(const DiGraph& x): nodecnt(0) {asgRepGraph(*this,x);}
    BitRep(const BiDirectionalGraph& x): nodecnt(0) {asgRepGraph(*this,x);}
    BitRep(const NautyRep& x): nodecnt(0) {asgRep(*this,x);}
    BitRep(const BiDirectionalBitRep& x): nodecnt(0) {asgRep(*this,x);}
    const BitRep& operator=(const DiGraph& x) {asgRepGraph(*this,x); return *this;}
    const BitRep& operator=(const BiDirectionalGraph& x) {
      asgRepGraph(*this,x); return *this;}
    const BitRep& operator=(const NautyRep& x) {asgRep(*this,x); return *this;}
    const BitRep& operator=(const BiDirectionalBitRep& x) 
    {asgRep(*this,x); return *this;}

    const BitRep& operator|=(const BitRep& x) 
    {linklist|=x.linklist; return *this;}
    BitRep operator|(BitRep x) const {return x|=*this;}
    const BitRep& operator&=(const BitRep& x) 
    {linklist&=x.linklist; return *this;}
    BitRep operator&(BitRep x) const {return x&=*this;}
    const BitRep& operator^=(const BitRep& x) 
    {linklist^=x.linklist; return *this;}
    BitRep operator^(BitRep x) const {return x^=*this;}
    BitRep operator~() const
    {BitRep r; r.nodecnt=nodecnt; r.linklist=~linklist; return r;}

    BitRep operator+=(const BitRep& x) {
      linklist|=x.linklist>>nodecnt;
      nodecnt+=x.nodecnt;
      return *this;
    }
    BiDirectionalBitRep symmetrise(); 

    bitref operator()(unsigned i, unsigned j) {
      return linklist[idx(i,j)];}
    const_bitref operator()(unsigned i, unsigned j) const {
      return linklist[idx(i,j)];}
    /// number of nodes this graph has
    unsigned nodes() const {return nodecnt;} /* number of nodes */
    /// number of links this graph has
    unsigned links() const {return linklist.popcnt();}
    /// next permutation of links - returns false if no further permutations
    bool next_perm() {return linklist.next_perm();}
    bool operator==(const BitRep& x) const {
      return nodecnt==x.nodecnt && linklist==x.linklist;
    }
    bool operator!=(const BitRep& x) const {return !operator==(x);}
    bool operator<(const BitRep& x) const {
      return nodecnt<x.nodecnt || (nodecnt==x.nodecnt && linklist<x.linklist);
    }
    bool contains(const Edge& e) const {return operator()(e.source(),e.target());}
    void push_back(const Edge& e) {operator()(e.source(),e.target())=true;}
    void clear (unsigned nodes=0) {
      nodecnt=nodes; linklist.setrange( 0, nodes * (nodes-1), false);}
  };

  /**
     a directed graph with the linklist represented by a bitvector
  */
  class NautyRep
  {
    unsigned nodecnt;
    CLASSDESC_ACCESS(NautyRep);
    friend void ecolab_nauty(const NautyRep&,NautyRep&,double&,bool,int);
  public:
    typedef Rep_const_iterator<NautyRep> const_iterator;
    const_iterator begin() const {return const_iterator(*this,false);}
    const_iterator end() const {return const_iterator(*this,true);}

    unsigned m() const {return SETWD(nodecnt-1)+1;}
    unsigned mw() const {return ECOLAB_WORDSIZE*m();}
    bitvect linklist;

    NautyRep(): nodecnt(0) {}
    NautyRep(unsigned nodes): 
      nodecnt(nodes),linklist( nodes * mw(), false)  {
    }
    NautyRep(const Graph& x): nodecnt(0) {asgRepGraph(*this,x);}
    NautyRep(const DiGraph& x): nodecnt(0) {asgRepGraph(*this,x);}
    NautyRep(const BiDirectionalGraph& x): nodecnt(0) {asgRepGraph(*this,x);}
    NautyRep(const BitRep& x): nodecnt(0) {asgRep(*this,x);}
    NautyRep(const BiDirectionalBitRep& x): nodecnt(0) {asgRep(*this,x);}
    const NautyRep& operator=(const Graph& x) {
      asgRepGraph(*this,x); return *this;}
    const NautyRep& operator=(const DiGraph& x) {
      asgRepGraph(*this,x); return *this;}
    const NautyRep& operator=(const BiDirectionalGraph& x) {
      asgRepGraph(*this,x); return *this;}
    const NautyRep& operator=(const BitRep& x) {asgRep(*this,x); return *this;}
    const NautyRep& operator=(const BiDirectionalBitRep& x) {
      asgRep(*this,x); return *this;}

    //  BiDirectionalBitRep symmetrise();
    bitref operator()(unsigned i, unsigned j) {
      return linklist[mw()*i+j];}
    const_bitref operator()(unsigned i, unsigned j) const {
      return linklist[mw()*i+j];}
    /// number of nodes this graph has
    unsigned nodes() const {return nodecnt;} /* number of nodes */
    /// number of links this graph has
    unsigned links() const {return linklist.popcnt();}
    /// next permutation of links - returns false if no further permutations
    bool operator==(const NautyRep& x) const {
      return nodecnt==x.nodecnt && linklist==x.linklist;
    }
    bool operator!=(const NautyRep& x) const {return !operator==(x);}
    bool operator<(const NautyRep& x) const {
      return nodecnt<x.nodecnt || (nodecnt==x.nodecnt && linklist<x.linklist);
    }
    /// log of the number of bitstrings equivalent to this graph
    double lnomega() const;
    /// canonical form of this graph - same graph is return for all
    /// equivalent graphs
    NautyRep canonicalise() const;
    bool contains(const Edge& e) const {return operator()(e.source(),e.target());}
    void push_back(const Edge& e) {operator()(e.source(),e.target())=true;}
    void clear (unsigned nodes=0) {
      nodecnt=nodes; linklist.setrange( 0, nodes * (nodes-1), false);}
  };

  /**
     an undirected graph with the linklist represented by a bitvector
  */
  class BiDirectionalBitRep
  {
    bitvect linklist;
    unsigned nodecnt;
    unsigned idx(unsigned i, unsigned j) const {
      return (j<i)? ((i*(i-1))>>1) + j: ((j*(j-1))>>1)+i;
    }
    CLASSDESC_ACCESS(BiDirectionalBitRep);
  public:
    typedef Rep_const_iterator<BiDirectionalBitRep> const_iterator;
    const_iterator begin() const {return const_iterator(*this,false);}
    const_iterator end() const {return const_iterator(*this,true);}

    operator BitRep() const {BitRep r; asgRep(r,*this); return r;}
    /// generate a node graph with l links at start of bit sequence (eg
    /// 11111000000....)
    BiDirectionalBitRep(unsigned nodes=0, unsigned links=0): 
      linklist( nodes*(nodes-1)/2, false), nodecnt(nodes) {
      assert(links<=linklist.size());
      linklist.setrange(0,links,true);
    }
    BiDirectionalBitRep(const Graph& x): nodecnt(0) {asgRepGraph(*this,x);}
    BiDirectionalBitRep(const DiGraph& x): nodecnt(0) {asgRepGraph(*this,x);}
    BiDirectionalBitRep(const BiDirectionalGraph& x): nodecnt(0) {
      asgRepGraph(*this,x);}
    BiDirectionalBitRep(const NautyRep& x): nodecnt(0) {asgRep(*this,x);}
    BiDirectionalBitRep(const BitRep& x): nodecnt(0) {asgRep(*this,x);}
    const BiDirectionalBitRep& operator=(const DiGraph& x) 
    {asgRepGraph(*this,x); return *this;}
    const BiDirectionalBitRep& operator=(const BiDirectionalGraph& x) 
    {asgRepGraph(*this,x); return *this;}
    const BiDirectionalBitRep& operator=(const NautyRep& x) 
    {asgRep(*this,x); return *this;}
    const BiDirectionalBitRep& operator=(const BitRep& x) 
    {asgRep(*this,x); return *this;}

    bitref operator()(unsigned i, unsigned j) {
      return linklist[idx(i,j)];}
    const_bitref operator()(unsigned i, unsigned j) const {
      return linklist[idx(i,j)];}
    /// number of nodes this graph has
    unsigned nodes() const {return nodecnt;} /* number of nodes */
    /// number of links this graph has
    unsigned links() const {return 2*linklist.popcnt();}
    /// next permutation of links - returns false if no further permutations
    bool next_perm() {return linklist.next_perm();}
    bool operator==(const  BiDirectionalBitRep& x) const {
      return nodecnt==x.nodecnt && linklist==x.linklist;
    }
    bool operator!=(const BiDirectionalBitRep& x) const {return !operator==(x);}
    bool operator<(const BiDirectionalBitRep& x) const {
      return nodecnt<x.nodecnt || (nodecnt==x.nodecnt && linklist<x.linklist);
    }
    bool contains(const Edge& e) const {return operator()(e.source(),e.target());}
    void push_back(const Edge& e) {operator()(e.source(),e.target())=true;}
    void clear (unsigned nodes=0) {
      nodecnt=nodes; linklist.setrange( 0, nodes * (nodes-1), false);}
  };

  template <> inline 
  bool GraphAdaptor<BiDirectionalBitRep>::directed() const {return false;}

  BitRep permute(const BitRep& x, const vector<unsigned>& perm);

  /**
     \brief EcoLab interface to Nauty
     @param g Graph the analyse
     @param canonical returned canonical representation
     @param lnomega \f$\ln\Omega\f$
     @param do_canonical whether to compute a canonical rep, or just \f$\ln\Omega\f$
  */
  void ecolab_nauty(const NautyRep& g, NautyRep& canonical, double& lnomega, 
                    bool do_canonical, int invMethod=0);

  // call saucy for lnomega (if available)
  double saucy_lnomega(const DiGraph&);
  // compute lnomega using bliss \a method 
  double igraph_lnomega(const DiGraph& g, int method);

  inline NautyRep canonicalise(const NautyRep& x) {return x.canonicalise();}

  /* the above operators allow TCL_obj to work */
#ifdef _CLASSDESC
#pragma omit TCL_obj BitRep
#pragma omit TCL_obj BiDirectionalBitRep
#endif

  BitRep start_perm(unsigned nodes, unsigned links);
  double lnfactorial(unsigned x);
  double lnCombine(unsigned x, unsigned y);

  inline double baseComplexity(unsigned n,unsigned l, bool bidirectional=false)
  {
    double ilog2=1/log(2.0);
    unsigned  N=n*(n-1);
    //cout << "lnCombine("<<N<<","<<l<<")="<<lnCombine(N,l)<<", lnomega="<<x.lnomega()<<endl;
    return 2*ceil(log(n+1)*ilog2) + 1 + ceil(log(N)*ilog2) + 
      ilog2 * (bidirectional? lnCombine(N/2,l/2): lnCombine(N,l));
  }
  
  inline double complexity(NautyRep x, bool bidirectional=false)
  { 
    double ilog2=1/log(2.0);
    //  cout << "NautyRep omega="<<exp(x.lnomega())<<endl;
    return baseComplexity(x.nodes(),x.links(),bidirectional) - ilog2 * x.lnomega();
  }

  /** 
      \brief SuperNOVA automorphism algorithm:
      @param canon canonical form of the input graph, including a reordered 
      attribute vector
      @param g input graph
      @return the ln omega of the graph
  */
  double canonical(BitRep& canon, const Graph& g);

  template <class G> double canonical(BitRep& canon, const GraphAdaptor<G>& g)
  {return canonical(canon, static_cast<const Graph&>(g));}

  template <class G> double canonical(BitRep& canon, const ConcreteGraph<G>& g)
  {return canonical(canon, static_cast<const Graph&>(g));}

  template <class G> double canonical(BitRep& canon, const G& g)
  {return canonical(canon, GraphAdaptor<const G>(g));}

  /// Network complexity
  //@{
  double complexity(const Graph& g);
  template <class G> double complexity(const GraphAdaptor<G>& g)
  {return complexity(static_cast<const Graph&>(g));}

  double complexity(const Graph& g);
  template <class G> double complexity(const ConcreteGraph<G>& g)
  {return complexity(static_cast<const Graph&>(g));}

  template <class G> double complexity(const G& g)
  {return complexity(GraphAdaptor<const G>(g));}
  //@}

#if WORDSIZE==64
  // shift right bug (~0UL)>>64 gives the wrong answer
  inline setword bitrange(unsigned i) {return i==64? 0: ALLBITS>>i;}
#else
  // shift right bug (~0U)>>32 gives the wrong answer
  inline setword bitrange(unsigned i) {return i==32? 0: ALLBITS>>i;}
#endif

  inline bitvect num(const bitvect& x, unsigned start,unsigned nbits)
  {
    return x.num(start,nbits);
  }

  inline bitvect bitvect::num(unsigned start,unsigned nbits) const
  {
    bitvect r(nbits);
    if (start+nbits<ECOLAB_WORDSIZE) 
      r.data[0]= (data[0]<<start) & ~bitrange(nbits);
    else
      for (unsigned i=0; i<nbits; i++) 
        r[i]=(*this)[start+i];
    return r;
  }

  template <class T>
  vector<T>  num(const vector<T>& x, unsigned start,unsigned nbits)
  {return vector<T>(x.begin()+start,x.begin()+start+nbits);}

  inline bool equal(const bitvect& x, const bitvect& y, unsigned len, unsigned offs) 
  {
    return x.equal(y,len,offs);
  }

  inline bool bitvect::equal(const bitvect& y, unsigned len, unsigned offs) const
  {
    bool r=true;
    if (offs+len>ECOLAB_WORDSIZE)
      {
        unsigned i;
        for (i=0; r && i<div(len); i++)
          r&= data[i]==(y.data[div(offs)+i]<<mod(offs)|
            y.data[div(offs)+i+1]>>(ECOLAB_WORDSIZE-mod(offs)));
        unsigned lastword= (div(offs)+i+1<y.nwords())? 
          y.data[div(offs)+i+1]>>(ECOLAB_WORDSIZE-mod(offs)): 0;
        r&= !(~bitrange(mod(len)) & 
              (data[i] ^ (y.data[div(offs)+i]<<mod(offs)| lastword)));
      }
    else
      r&= !(~bitrange(len) & (data[0] ^ (y.data[0]<<offs)));
    return r;
  }

  template <class T>
  bool equal(const T& x, const T& y, unsigned len, unsigned offs)
  {
    //  return std::equal(x.begin(),x.begin()+len,y.begin()+offs);
    bool r=true; 
    for (unsigned i=0; r&&i<len; i++) 
      r&=x[i]==y[offs+i];
    return r;
  }

  template <class T>
  void setvec(vector<T>& x,const bitvect& y)
  {
    x.resize(y.size()); 
    for (unsigned i=0; i<y.size(); i++) 
      x[i]=y[i];
  }

  /// Offdiagonal complexity
  double ODcomplexity(const DiGraph& x);

  /// Medium articulation
  double MA(const DiGraph& x);

  inline BiDirectionalBitRep BitRep::symmetrise()
  {BiDirectionalBitRep r; asgRep(r,*this); return r;}

} //namespace ecolab

ostream& operator<<(ostream& s, const ecolab::bitvect& x);
inline ostream& operator<<(ostream& s, const ecolab::BitRep& x) 
{return s<<ecolab::DiGraph(x);}
inline ostream& operator<<(ostream& s, const ecolab::BiDirectionalBitRep& x) 
{return s<<ecolab::BiDirectionalGraph(x);}
inline ostream& operator<<(ostream& s, const ecolab::NautyRep& x) 
{return s<<ecolab::DiGraph(x);}

#include "netcomplexity.cd"
#endif
