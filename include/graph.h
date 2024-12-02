/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief EcoLab graph library
*/
#ifndef GRAPH_H
#define GRAPH_H

#include <classdesc_access.h>
#include "sparse_mat.h"
#include "pack_stl.h"
#include "arrays.h"
#include "poly.h"

#include <vector>
#include <set>
#include <utility>
#include <algorithm>
#include <iostream>
#include <assert.h>

#ifdef _CLASSDESC
#pragma omit pack ecolab::Graph::back_insert_iterator
#pragma omit unpack ecolab::Graph::back_insert_iterator
#pragma omit pack ecolab::Graph::const_iterator
#pragma omit unpack ecolab::Graph::const_iterator
#pragma omit pack ecolab::BiDirectionalGraph::const_iterator
#pragma omit unpack ecolab::BiDirectionalGraph::const_iterator
#pragma omit pack ecolab::sparse_mat_graph_adaptor::const_iterator
#pragma omit unpack ecolab::sparse_mat_graph_adaptor::const_iterator
#pragma omit pack ecolab::sparse_mat_graph_adaptor
#pragma omit unpack ecolab::sparse_mat_graph_adaptor

#endif

namespace ecolab
{
  struct Edge: public std::pair<unsigned,unsigned>
  {
    typedef std::pair<unsigned,unsigned> super;
    unsigned& source() {return first;}
    unsigned& target() {return second;}
    unsigned source() const {return first;}
    unsigned target() const {return second;}
    float weight;
    Edge(): weight(1) {}
    template<class S, class T>
    Edge(S source, T target): super(source, target), weight(1.0) {
      assert(source!=target);}
    template<class S, class T, class W>
    Edge(S source, T target, W weight):
      super(source, target), weight(weight) {
      assert(source!=target);}

    // edges should be unique wrt their connections only. 
    bool operator==(const Edge& e) const {
      return first==e.first && second==e.second;
    }
    bool operator<(const Edge& e) const {
      return first<e.first || (first==e.first && second<e.second);
    }
  };

  /// To support back_insert_iterators for Graph, define a special
  /// version that converts from BoostGraph edges
  template <class Graph, class BG>
  class Graph_back_insert_iterator/*: 
                                    public std::iterator<std::output_iterator_tag, void, void, void, void>*/
  {
    Graph& g;
    const BG& bg;
  public:
    Graph_back_insert_iterator(Graph& g, const BG& bg): g(g), bg(bg) {}
    Graph_back_insert_iterator& operator=
    (const typename BG::edge_iterator::value_type& bge)
    {
      g.push_back(Edge(source(bge,bg),target(bge,bg)));
      return *this;
    }
    Graph_back_insert_iterator& operator*() {return *this; }
    Graph_back_insert_iterator& operator++() {return *this; }
  };

  /**
     Abstract base class for graph algorithms. 
  */
  struct Graph
  {
    typedef Edge value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef Edge& reference;
    typedef const Edge& const_reference;

    struct const_iterator_base: public classdesc::object
    {
      virtual Edge operator*() const=0;
      virtual const const_iterator_base& operator++()=0;
      virtual bool operator==(const const_iterator_base& x) const=0;
      virtual ~const_iterator_base() {}
    };

    /// iterator over edges
    class const_iterator: public classdesc::poly<const_iterator_base> /*,
                          public std::iterator<std::forward_iterator_tag,Edge>*/
    {
      typedef classdesc::poly<Graph::const_iterator_base> super;
      Graph::const_iterator_base& base() {return super::operator*();}
      const Graph::const_iterator_base& base() const {return super::operator*();}
      CLASSDESC_ACCESS(const_iterator);
      mutable Edge edge; //used for operator->
    public:
      Edge operator*() const {
        return *base();
      }
      const Edge* operator->() const {edge=*base(); return &edge;}
      const_iterator& operator++() {++base(); return *this;}
      bool operator==(const const_iterator& x) const {return base()==x.base();}
      bool operator!=(const const_iterator& x) const {return !operator==(x);}
    };

    /// iterator to first edge
    virtual const_iterator begin() const=0;
    /// iterator beyond last edge
    virtual const_iterator end() const=0;
    /// number of nodes
    virtual unsigned nodes() const=0;
    /// number of links
    virtual unsigned links() const=0;
    /// add an edge. Node count is adjusted so that edge is valid.
    virtual void push_back(const Edge& e)=0;
    /// support for Boost Graph Library
    template <class BG> void push_back
    (const typename BG::edge_iterator::value_type& e)
    {push_back(Edge(e.first,e.second));}
    /// true if graph contains \a e
    virtual bool contains(const Edge& e) const=0;
    /// true if a bidirectional graph (all edges go both ways)
    virtual bool directed() const=0;
    /// remove all edges from the graph and set the node count
    virtual void clear(unsigned nodes=0)=0;
    const Graph& operator=(const Graph& x) {
      clear(x.nodes());
      for (const_iterator e=x.begin(); e!=x.end(); ++e)
        push_back(*e);
      return *this;
    }
    virtual ~Graph() {}
    ///@{ available formats pajek, lgl, dot, gengraph (input only)
    void input(const std::string& format, const std::string& filename);
    void output(const std::string& format, const std::string& filename) const;
    /// @}

    /// output iterator for constructing a Graph object from a boost
    /// graph algorithm
    template <class BG>
    Graph_back_insert_iterator<Graph,BG> back_inserter(const BG& bg) {
      return Graph_back_insert_iterator<Graph,BG>(*this,bg);
    }
  };

  // define this outside GraphAdaptor to get around classdesc bug.
  template <class G> struct GraphAdaptor_const_iterator_base: 
    public G::const_iterator,
    public classdesc::Object<
    GraphAdaptor_const_iterator_base<G>, Graph::const_iterator_base
    >
  {
    GraphAdaptor_const_iterator_base(const typename G::const_iterator& i): 
      G::const_iterator(i) {}
    Edge operator*() const override {return G::const_iterator::operator*();}
    const Graph::const_iterator_base& operator++() override {
      G::const_iterator::operator++(); return *this;}
    bool operator==(const GraphAdaptor_const_iterator_base& x) const {
      return static_cast<const typename G::const_iterator&>(*this)==x;
    }
    bool operator==(const Graph::const_iterator_base& x) const override {
      const GraphAdaptor_const_iterator_base* xp=
        dynamic_cast<const GraphAdaptor_const_iterator_base*>(&x);
      return xp && static_cast<const typename G::const_iterator&>(*this)==static_cast<const typename G::const_iterator&>(*xp);
    }
  };

#ifdef _CLASSDESC
#pragma omit pack ecolab::Graph::const_iterator_base
#pragma omit unpack ecolab::Graph::const_iterator_base
#pragma omit pack ecolab::GraphAdaptor_const_iterator_base
#pragma omit unpack ecolab::GraphAdaptor_const_iterator_base
#pragma omit TCL_obj ecolab::GraphAdaptor_const_iterator_base
#pragma omit isa ecolab::GraphAdaptor_const_iterator_base
#endif

/**
   an adaptor (wrapper) around an existing graph object that converts
   it into a polymorphic Graph object. The existing graph object must
   "duck type" the abstract base class.
*/
template <class G>
class GraphAdaptor: public Graph
{
  G& g;
  CLASSDESC_ACCESS(GraphAdaptor);

  typedef GraphAdaptor_const_iterator_base<G> const_iterator_base;
  // a bit a mucking around to get this template to work for <const G>
  template <class GG> void Gpush(GG& g, const Edge& e) {g.push_back(e);}
  template <class GG> void Gpush(const GG& g, const Edge& e) {}
  template <class GG> void Gclear(GG& g, unsigned nodes) {g.clear(nodes);}
  template <class GG> void Gclear(const GG& g, unsigned nodes) {}

public:
  GraphAdaptor(G& g): g(g) {}
  const GraphAdaptor& operator=(const GraphAdaptor& x) {g=x.g; return *this;}

  Graph::const_iterator begin() const {
    Graph::const_iterator r;
    r.addObject<const_iterator_base>(g.begin());
    return r;
  }
  Graph::const_iterator end() const {
    Graph::const_iterator r;
    r.addObject<const_iterator_base>(g.end());
    return r;
  }
 
  unsigned nodes() const {return g.nodes();}
  unsigned links() const {return g.links();}
  bool contains(const Edge& e) const {return g.contains(e);}
  bool directed() const;
  void push_back(const Edge& e) {Gpush(g,e);}
  void clear(unsigned nodes=0) {Gclear(g,nodes);}
  /// @{TCL I/O commands \a arg1=format, \a arg2=filename
//  void input(TCL_args arg) {Graph::input(arg[0], arg[1]);} 
//  void output(TCL_args arg) const {Graph::output(arg[0], arg[1]);}
  /// @}

};  

#ifdef _CLASSDESC
//#pragma omit pack GraphAdaptor
//#pragma omit unpack GraphAdaptor
#endif

template <class G>
class ConcreteGraph: public GraphAdaptor<G>
{
  G g;
  CLASSDESC_ACCESS(ConcreteGraph);
public:
  ConcreteGraph(unsigned nodes=0): GraphAdaptor<G>(g), g(nodes) {}
  template <class H> explicit ConcreteGraph(const H& g1): GraphAdaptor<G>(g), g(g1) {}
  bool operator==(const ConcreteGraph& x) {return x.g==g;}
  bool operator!=(const ConcreteGraph& x) {return x.g!=g;}
};

// default is not directed
template <class G> bool GraphAdaptor<G>::directed() const
{return true;}

class DiGraph: private std::set<Edge>
{
  unsigned num_nodes;
  CLASSDESC_ACCESS(DiGraph);
public:
  typedef std::set<Edge> Evec;
  // export all const functions of the underlying vector
  using Evec::size;
  using Evec::empty;
  using Evec::begin;
  using Evec::end;
  using Evec::value_type;
  using Evec::size_type;
  using Evec::difference_type;
  //using Evec::const_iterator;
  using Evec::const_reference;

  struct const_iterator: public Evec::const_iterator
  {
    const_iterator() {}
    const_iterator(const Evec::const_iterator& x): Evec::const_iterator(x) {}
#ifdef __APPLE_CC__
  // bizarrely iterator equality is missing on Apple's compiler!
    bool operator==(const const_iterator& x) const {return &**this==&*x;}
#endif
  };

  using iterator=const_iterator; //disallow modification of edges
    
  unsigned nodes() const {return num_nodes;}
  unsigned links() const {return size();}
  bool contains(const Edge& e) const {return count(e);}
  DiGraph(unsigned nodes=0): num_nodes(nodes) {}
  /// initialise Graph using Graph "duck-typed" object
  template <class G> explicit DiGraph(const G& g): num_nodes(0) {asg(g);}
  template <class G> void asg(const G& g) {
    clear(g.nodes());
    for (typename G::const_iterator i=g.begin(); i!=g.end(); ++i) push_back(*i);
  } 
  // use push_back for graph construction
  void push_back(const Edge& e) {
    if (e.source()>=num_nodes) num_nodes=e.source()+1;
    if (e.target()>=num_nodes) num_nodes=e.target()+1;
    Evec::insert(e);
  }
  void clear(unsigned nodes=0) {num_nodes=nodes; Evec::clear();}
  void swap(DiGraph& g) {std::swap(num_nodes,g.num_nodes); Evec::swap(g);}
  bool operator==(const DiGraph& x) const
  {return static_cast<const Evec&>(*this)==static_cast<const Evec&>(x);}
  bool operator!=(const DiGraph& x) const {return !operator==(x);}
  bool operator<(const DiGraph& x) const
  {return static_cast<const Evec&>(*this)<static_cast<const Evec&>(x);}
};

#ifdef _CLASSDESC
#pragma omit pack ecolab::DiGraph::const_iterator
#pragma omit unpack ecolab::DiGraph::const_iterator
#pragma omit TCL_obj ecolab::DiGraph::const_iterator
#pragma omit isa ecolab::DiGraph::const_iterator
#pragma omit pack ecolab::BiDirectionalGraph_const_iterator
#pragma omit unpack ecolab::BiDirectionalGraph_const_iterator
#pragma omit TCL_obj ecolab::BiDirectionalGraph_const_iterator
#pragma omit isa ecolab::BiDirectionalGraph_const_iterator
#endif

class BiDirectionalGraph_const_iterator/*:
                                         public std::iterator<std::forward_iterator_tag,Edge>*/
{
  bool first;
  DiGraph::const_iterator it;
  CLASSDESC_ACCESS(BiDirectionalGraph_const_iterator);
  mutable Edge swapped;
public:
  BiDirectionalGraph_const_iterator(const DiGraph::const_iterator& i): first(true), it(i)  {}
  const Edge& operator*() const {
    if (first) return *it;
    else 
      {
        swapped=Edge(it->target(),it->source(), it->weight);
        return swapped;
      }
  }
  const Edge* operator->() const {return &operator*();}
  
  BiDirectionalGraph_const_iterator& operator++() 
  {first=!first; if (first) ++it; return *this;}
  BiDirectionalGraph_const_iterator operator++(int) 
  {BiDirectionalGraph_const_iterator old(*this); operator++(); return old;}
  BiDirectionalGraph_const_iterator& operator--() 
  {first=!first; if (!first) --it; return *this;}
  BiDirectionalGraph_const_iterator operator--(int) 
  {BiDirectionalGraph_const_iterator old(*this); operator--(); return old;}
  bool operator==(const BiDirectionalGraph_const_iterator& i) const 
  {return first==i.first && it==i.it;}
  bool operator!=(const BiDirectionalGraph_const_iterator& i) const 
  {return !operator==(i);}
};

/// A graph in which each link is bidirectional
// base class is protected, because viewing this thing as a Graph is not correct
class BiDirectionalGraph
{
  DiGraph graph; ///< the representation
  CLASSDESC_ACCESS(BiDirectionalGraph);
public:
  // iterators return reverse link after normal link
  typedef BiDirectionalGraph_const_iterator const_iterator;
  typedef const_iterator iterator; // cannot update individual links
  const_iterator begin() const {return const_iterator(graph.begin());}
  const_iterator end() const {return const_iterator(graph.end());}
  // use push_back for graph construction
  void push_back(Edge e) {
    if (e.source()>e.target()) std::swap(e.source(),e.target());
    graph.push_back(e);
  }
  void clear(unsigned nodes=0) {graph.clear(nodes);}

  BiDirectionalGraph(unsigned nodes=0): graph(nodes) {}
  /// initialise Graph using Graph "duck-typed" object
  template <class G> BiDirectionalGraph(const G& g) {asg(g);}
  template <class G> void asg(const G& g) {
    clear(g.nodes());
    for (typename G::const_iterator i=g.begin(); i!=g.end(); ++i) push_back(*i);
  } 
  unsigned nodes() const {return graph.nodes();}
  unsigned links() const {return 2*graph.links();}
  bool contains(Edge e) const {
    if (e.source()>e.target()) std::swap(e.source(),e.target());
    return graph.contains(e);
  }
  void swap(BiDirectionalGraph& g) {graph.swap(g.graph);}
  bool operator==(const BiDirectionalGraph& x) const {return graph==x.graph;}
  bool operator!=(const BiDirectionalGraph& x) const {return graph!=x.graph;}
  bool operator<(const BiDirectionalGraph& x) const {return graph<x.graph;}
};

template <> inline 
bool GraphAdaptor<BiDirectionalGraph>::directed() const {return false;}

// uses the sign of the off-diagonal term to indicate link
// direction. Links point in the same direction have their weights
// summed
class sparse_mat_graph: public ConcreteGraph<DiGraph>
{
public:
  template <class F>
  const sparse_mat_graph& operator=(const sparse_mat<F>& mat) {
    clear();
    std::map<std::pair<size_t, size_t>, double > weights;
    for (size_t i=0; i<mat.row.size(); ++i) {
      if (mat.val[i] > 0)
        weights[std::make_pair(mat.row[i],mat.col[i])]+=mat.val[i];
      else
        weights[std::make_pair(mat.col[i],mat.row[i])]-=mat.val[i];
    }
    std::map<std::pair<size_t, size_t>, double >::iterator i = weights.begin();
    for (; i!=weights.end(); ++i)
      push_back(Edge(i->first.first, i->first.second, i->second));
    return *this;
  }
};

//An adaptor for sparse_mats. Uses absolute values for weights
template <class F>
class sparse_mat_graph_adaptor
{
  const sparse_mat<F>& mat;
  CLASSDESC_ACCESS(sparse_mat_graph_adaptor);
public:
  sparse_mat_graph_adaptor(const sparse_mat<F>& mat): mat(mat) {}
  
  class const_iterator
  {
    size_t it;
    const sparse_mat<F>& mat;
    CLASSDESC_ACCESS(const_iterator);
    mutable Edge link;
  public:
    const_iterator(const sparse_mat<F>& mat, size_t it): it(it), mat(mat)  {}
    const Edge& operator*() const 
    {
      // we're using abs here, as networks need to have positive
      // weights for compelxity calculations.  not sure if we'll
      // continue with this concept, or adjust it in complexity
      link=Edge(mat.row[it], mat.col[it], std::abs(mat.val[it])); 
      return link;
    }
    const Edge* operator->() const {return &operator*();}
    const_iterator& operator++() {++it; return *this;}
    const_iterator operator++(int) {++it; return const_iterator(mat,it-1);}
    const_iterator& operator--() {--it; return *this;}
    const_iterator operator--(int) {--it; return const_iterator(mat,it+1);}
    bool operator==(const const_iterator& i) const {return it==i.it && &mat==&i.mat;}
    bool operator!=(const const_iterator& i) const {return !operator==(i);}
  };
  const_iterator begin() const {return const_iterator(mat,0);}
  const_iterator end() const {return const_iterator(mat,mat.row.size());}
  
  unsigned nodes() const {assert(mat.rowsz==mat.colsz); 
    return mat.diag.size()==0? mat.rowsz: mat.diag.size();}
  unsigned links() const {return(mat.row.size());}
  bool contains(const Edge& e) const {
    for (size_t i=0; i<mat.row.size(); ++i)
      if (mat.row[i]==e.source() && mat.col[i]==e.target())
        return true;
    return false;
  }
};

inline void swap(DiGraph& x, DiGraph& y) {x.swap(y);}
inline void swap(BiDirectionalGraph& x, BiDirectionalGraph& y) {x.swap(y);}

struct Degrees
{
  ecolab::array_ns::array<unsigned> in_degree, out_degree;
  Degrees(unsigned nodes=0) : in_degree(nodes,0), out_degree(nodes,0) {}
};
 
template<class G>
Degrees degrees(const G& g)
{
  Degrees d(g.nodes());
  for (typename G::const_iterator e=g.begin(); e!=g.end(); ++e)
    {
      d.in_degree[e->target()]++;
      d.out_degree[e->source()]++;
    }
  return d;
}

///default indegree distribution (builds trees)
static struct ConstantRNG: public random_gen
{
  double rand() {return 1;}
} one;


/// random graph chosen uniformly from the set of graphs with n nodes and l links
void ErdosRenyi_generate(Graph& g, unsigned nodes, unsigned links, urand& uni,
                         random_gen& weight_dist=one);

/// Barabasi-Albert Preferential Attachment model. \a indegree_dist is
/// the distribution from which the in degree is drawn for each new
/// node.
void PreferentialAttach_generate(Graph& g, unsigned nodes, urand& uni,
                                 random_gen& indegree_dist=one, random_gen& weight_dist=one);

/// randomly rewire the graph \a g (in place), using random generator \a u
void random_rewire(Graph& g, urand& u);

} // namespace ecolab

namespace classdesc_access
{
  template <class G> struct access_pack
  <ecolab::GraphAdaptor_const_iterator_base<G> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class G> struct access_unpack
  <ecolab::GraphAdaptor_const_iterator_base<G> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
}

namespace std
{
  /// for use with TCL_obj. Graphviz format is used with the netgraph command.
  std::ostream& operator<<(std::ostream& s, const ecolab::Graph& x);
  template <class G>
  std::ostream& operator<<(std::ostream& s, const ecolab::ConcreteGraph<G>& x)
  {return s<<static_cast<const ecolab::Graph&>(x);}

  inline std::ostream& operator<<(std::ostream& s, const ecolab::DiGraph& x)
  {return s<<ecolab::GraphAdaptor<const ecolab::DiGraph>(x);}
  inline std::ostream& operator<<(std::ostream& s, 
                                const ecolab::BiDirectionalGraph& x)
  {return s<<ecolab::GraphAdaptor<const ecolab::BiDirectionalGraph>(x);}

  std::istream& operator>>(std::istream& s, ecolab::Graph& x);
  template <class G>
  std::istream& operator>>(std::istream& s, ecolab::ConcreteGraph<G>& x)
  {return s>>static_cast<ecolab::Graph&>(x);}

  inline std::istream& operator>>(std::istream& s, ecolab::DiGraph& x)
  {ecolab::GraphAdaptor<ecolab::DiGraph> g(x); return s>>g;}
  inline std::istream& operator>>(std::istream& s, ecolab::BiDirectionalGraph& x)
  {ecolab::GraphAdaptor<ecolab::BiDirectionalGraph> g(x); return s>>g;}
}

#if defined(__GNUC__) && !defined(__ICC) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#include "graph.cd"

#if defined(__GNUC__) && !defined(__ICC) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif
