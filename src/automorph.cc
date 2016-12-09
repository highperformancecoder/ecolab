/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <graph.h>
#include <netcomplexity.h>
#include <ref.h>
#include "ecolab_epilogue.h"

#include <vector>
#include <utility>
using namespace std;
#include <arrays.h>
using namespace ecolab;
using classdesc::ref;
using array_ns::array;

namespace
{
  using ::operator<<;

  typedef vector<unsigned> UnsignedArray;
  typedef pair<unsigned,unsigned> Range;

  /**
     Used to attach information about nodes. The particular use case we
     have is attaching information about which fixed nodes a node might
     be connected to in the Star Exploder algorithm.
  */
  typedef vector<Range> Attributes;

  inline ostream& operator<<(ostream& o, const Attributes& a)
  {
    for (unsigned i=0; i<a.size(); ++i)
      o<<a[i].first<<"-"<<a[i].second<<" ";
    return o;
  }

  struct MappedVertex
  {
    static unsigned next;
    unsigned id;
    MappedVertex(): id(next++) {}
  };

  typedef std::map<unsigned,MappedVertex> VertexMap;

  /** 
      a BitRep structure with nodal attributes
  */
  struct AttributedBitRep: public BitRep
  {
    Attributes attributes;
    AttributedBitRep(unsigned nodes=0, const Attributes& attr=Attributes()): 
      BitRep(nodes), attributes(attr) {}
    AttributedBitRep(const BitRep& bitRep, const Attributes& attr=Attributes()): 
      BitRep(bitRep), attributes(attr) {}

    /// include attributes into the ordering relationship
    bool operator<(const AttributedBitRep& x) const {
      return BitRep::operator<(x) || 
                              (BitRep::operator==(x) && attributes<x.attributes);
    }
  };

  /** used for graph colouring */
  struct GraphWithC: public AttributedBitRep
  {
    double lnomega;
    UnsignedArray remap;
    GraphWithC(): lnomega(0) {}
    GraphWithC(unsigned nodes, const Attributes& attr): 
      AttributedBitRep(nodes,attr), lnomega(0) {}
    void assign(const DiGraph& g, const Attributes& attr);
    bool operator<(const GraphWithC& x) const 
    {return AttributedBitRep::operator<(x);}
  };

  /** used for graph colouring */
  class ReducedGraph: public DiGraph
  {
    mutable GraphWithC g; //< reduced graph
    unsigned m_min_node; //< minimum node label (identifies component)
    CLASSDESC_ACCESS(ReducedGraph);
    mutable map<unsigned,unsigned> canonVertexMap; 
  public:
    mutable Attributes attributes; //<attribute information attached to nodes
    GraphWithC graph() const // return  reduced graph
    {
      if (g.nodes()==0) 
        {
          if (DiGraph::nodes())
            {
              //renumber vertices to eliminate disconnected vertices
              MappedVertex::next=0;
              VertexMap vertexMap; 
              DiGraph tg;
              for (const_iterator e=begin(); e!=end(); ++e)
                tg.push_back(Edge(vertexMap[e->source()].id,vertexMap[e->target()].id));
              // ensure attribute vector covers all nodes
              attributes.resize(DiGraph::nodes());
              Attributes attr(tg.nodes());
              for (VertexMap::iterator i=vertexMap.begin(); i!=vertexMap.end(); ++i)
                attr[i->second.id]=attributes[i->first];
              g.assign(tg,attr);
              // now that the canonical form of the reduced graph has been produced,
              // create a vertex map giving the canonical numbering
              vector<unsigned> invRemap(g.remap.size());
              for (unsigned i=0; i<invRemap.size(); ++i)
                invRemap[g.remap[i]]=i;
              for (VertexMap::iterator i=vertexMap.begin(); i!=vertexMap.end(); ++i)
                {
                  canonVertexMap[i->first]=invRemap[i->second.id];
                  assert(g.attributes[canonVertexMap[i->first]]==attributes[i->first]);
                }
            }
          else
            g=GraphWithC(1,attributes);
        }
      return g;
    }
    unsigned min_node() const {return m_min_node;}
    ReducedGraph(const DiGraph& g=DiGraph()): 
      DiGraph(g), m_min_node(std::numeric_limits<unsigned>::max()) {}
    void add_attributes(const Attributes& attr) {
      attributes=attr;
    }
    void push_back(const Edge& e) {
      DiGraph::push_back(e); 
      g=GraphWithC(); 
      m_min_node=std::min(m_min_node,std::min(e.source(),e.target()));
    }
    void merge(const ReducedGraph& x)
    {
      for (DiGraph::const_iterator e=x.begin(); e!=x.end(); ++e)
        push_back(*e);
    }
    unsigned canonicalVertex(unsigned i) const 
    {
      graph(); 
      assert(canonVertexMap.count(i)); 
      return canonVertexMap[i];
    }
  };

  typedef std::vector<classdesc::ref<ReducedGraph> > Component;
  typedef std::map<GraphWithC, unsigned> ComponentCount;

  struct AdjacencyMatrix: public vector<UnsignedArray>
  {
    AdjacencyMatrix() 
    {if (false) links();} //to remove spurious compiler warning
    AdjacencyMatrix(const DiGraph& g): vector<UnsignedArray>(g.nodes()) {
      for (DiGraph::iterator i=g.begin(); i!=g.end(); ++i)
        operator[](i->source()).push_back(i->target());
    }
    // reverse all edges of the graph
    AdjacencyMatrix tr() const {
      AdjacencyMatrix ret;
      ret.resize(size());
      for (unsigned i=0; i<size(); ++i)
        for (unsigned j=0; j<(*this)[i].size(); ++j)
          ret[(*this)[i][j]].push_back(i);
      return ret;
    }
    unsigned links() const {
      unsigned links=0;
      for (const_iterator i=begin(); i!=end(); ++i)
        links+=i->size();
      return links;
    }
  };

  struct OrderedCanonicalRange
  {
    vector<Range> range; ///< canonical ranges attached to nodes
    UnsignedArray nodeOrder;
    OrderedCanonicalRange() {}
    OrderedCanonicalRange(const vector<Range>& range, 
                          const UnsignedArray& nodeOrder): 
      range(range), nodeOrder(nodeOrder) {}

    const Range& operator[](unsigned i) const {return range[nodeOrder[i]];}
    Range& operator[](unsigned i) {return range[nodeOrder[i]];}
    unsigned size() const {return nodeOrder.size();}

//    bool operator<(const OrderedCanonicalRange& x) const {
//      for (unsigned i=0; i<nodeOrder.size(); ++i)
//        if (operator[](i) < x[i])
//          return true;
//        else if (operator[](i) > x[i])
//          return false;
//      return false;
//    }

//    bool operator==(const OrderedCanonicalRange& x) const {
//      for (unsigned i=0; i<nodeOrder.size(); ++i)
//        if (operator[](i) != x[i])
//          return false;
//      return true;
//    }

    void swap(OrderedCanonicalRange& x)
    {
      range.swap(x.range);
      nodeOrder.swap(x.nodeOrder);
    }
    bool consistent();
  };

#ifndef NDEBUG
  std::ostream& operator<<(std::ostream& o, const OrderedCanonicalRange& r)
  {
    for (unsigned i=0; i<r.size(); ++i)
      o << r[i].first<<"-"<<r[i].second << " ";
    return o;
  }

  bool OrderedCanonicalRange::consistent()   
  {
    bool r=operator[](0).first==0 && 
      operator[](size()-1).second==size();
    for (unsigned i=1; i<size(); ++i)
      r&=operator[](i-1)==operator[](i) ||
        operator[](i-1).second==operator[](i).first;
    if (!r) cout << *this << endl;
    return r;
  }
#endif
  
  template <class T>
  bool unique(const vector<T>& x)
  {
    set<T> s;
    for (unsigned i=0; i<x.size(); ++i)
      {
        pair<typename set<T>::iterator,bool> r=s.insert(x[i]);
        if (!r.second) return false;
      }
    return true;
  }

  struct CanonicalFinder
  {
    const DiGraph& g;
    const Attributes& attributes;
    AdjacencyMatrix a, trA;

    CanonicalFinder(const DiGraph& g, const Attributes& attr): 
      g(g), attributes(attr), a(g), trA(a.tr()) {
      assert(a.links()==g.links());
      assert(trA.links()==g.links());
    }

    /// Create a canonical range based on node degree
    void getInitialCanonicalRange(OrderedCanonicalRange& result);
    /// Fix \a node to the minimum of its range, and resort the remaining ranges
    void set1nodeAndResort(unsigned node, OrderedCanonicalRange& o);
    /// Return the bitRep corresponding to \a range. Only links
    /// between fixed nodes are included.
    AttributedBitRep bitRepOf(const OrderedCanonicalRange& range);
    /**
       Apply the star exploder algorithm to range
       @param [out] r log automorphism group size
       @param [out] result canonical graph
       @param [in] pos start applying algorithm from here
       @param [in/out] canonical range
       @return whether star exploder was successful or not
    */
    bool starExplode(double& r, AttributedBitRep& result, unsigned pos, 
                     OrderedCanonicalRange& range);
    /**
       apply symmetry break algorithm, ie for each variable node, fix
       it and resort. This algorithm is then applied recursively. The
       star exploder algorithm is applied at the start of each level
       of recursion, if the number of combinations exceeds a certain
       threshold. If not sucessful, the symmetry break algorithm is applied.

       @param [out] result canonical graph
       @param [in] pos position to start the algorithm
       @param [in/out] range canonical range
       @return log automorphism group size
    */
    double symmetryBreak(AttributedBitRep& result, unsigned pos, 
                                      OrderedCanonicalRange& range);
  };

  template <class Cmp>
  bool consistent(const Cmp& cmp, unsigned i, unsigned j)
  {return (cmp.less(i,j) && cmp.less(j,i)) == false;}

  /// comparison functional based on node degree (both in and out)
  class GreaterDegree
  {
    AdjacencyMatrix a, trA, dual;
    const Attributes& attributes;
  public:
    GreaterDegree(const DiGraph& g, const Attributes& attributes): 
      a(g), trA(a.tr()), attributes(attributes)
    {
      dual.resize(g.nodes());
      for (DiGraph::const_iterator i=g.begin(); i!=g.end(); ++i)
        if (g.contains(Edge(i->target(),i->source())))
          {
            dual[i->source()].push_back(i->target());
            dual[i->target()].push_back(i->source());
          }  
    }
    bool operator()(unsigned i, unsigned j) const {
      assert(consistent(*this,i,j));
      return less(i,j);
    }
    bool less(unsigned i, unsigned j) const {
      return a[i].size()>a[j].size() || 
        (a[i].size()==a[j].size() && 
         (trA[i].size()>trA[j].size() || 
          (trA[i].size()==trA[j].size() &&
           (dual[i].size() > dual[j].size() || 
            (dual[i].size() == dual[j].size() && attributes.size()>0 &&
             attributes[i]<attributes[j])))));
    }
  };

  /// comparison functional based on a node's neighbourhood (in or
  /// out), given an existing canonical range
  class GreaterNbrhd
  {
    AdjacencyMatrix nbrlabels;
    UnsignedArray labels;  
  public:
    GreaterNbrhd(const AdjacencyMatrix& a, const vector<Range>& clabels):
      labels(clabels.size()) {
      //Produce a sorted neighbour label list
      nbrlabels.resize(a.size());
      for (unsigned i=0; i<a.size(); ++i)
        {
          labels[i]=clabels[i].first;
        }
      for (unsigned i=0; i<a.size(); ++i)
        {
          nbrlabels[i].resize(a[i].size());
          UnsignedArray& nbr=nbrlabels[i];
          const UnsignedArray& ai=a[i];
          for (unsigned j=0; j<a[i].size(); ++j)
            nbr[j]=clabels[ai[j]].first;
          sort(nbrlabels[i].begin(), nbrlabels[i].end());
        }
    }
    // sort within existing sorted order according to neighbourhood
    bool operator()(unsigned i, unsigned j) const {
      assert(consistent(*this,i,j));
      assert(!(labels[i]<labels[j]) || less(i,j));
      return less(i,j);
    }
    bool less(unsigned i, unsigned j) const {
      return labels[i]<labels[j] ||
        (labels[i]==labels[j] && nbrlabels[i]<nbrlabels[j]);
    }
  };

  /// comparison functional based on a node's neighbourhood (both in and
  /// out), given an existing canonical range
  class GreaterInOutNbrhd
  {
    GreaterNbrhd outNbrhd, inNbrhd; 
  public:
    GreaterInOutNbrhd(const AdjacencyMatrix& a, const vector<Range>& clabels):
      outNbrhd(a,clabels), inNbrhd(a.tr(),clabels) {}
    bool operator()(unsigned i, unsigned j) const {
      assert(consistent(*this,i,j));
      return less(i,j);
    }
    bool less(unsigned i, unsigned j) const {
      return outNbrhd(i,j) || (!outNbrhd(j,i) && inNbrhd(i,j));
    }
  };

  /// comparison functional based on a node's neighbourhood (both in
  /// and out), given an existing canonical range, as well as a list
  /// of canonical components. This functional should never return an
  /// equivalent result (ie operator(i,j)|| operator(j,i) must be
  /// true) for two distinct indices i and j
  class GreaterInOutNbrhdWithComponent
  {
    GreaterInOutNbrhd ionbr;
    const Component& component;
  public:
    GreaterInOutNbrhdWithComponent
    (const AdjacencyMatrix& a, const vector<Range>& clabels, 
     const Component& component):
      ionbr(a,clabels), component(component) {}
    bool operator()(unsigned i, unsigned j) const {
      assert(consistent(*this,i,j));
      return less(i,j);
    }
    bool less(unsigned i, unsigned j) const {
      const classdesc::ref<ReducedGraph> &ci=component[i], &cj=component[j];
      return i!=j &&
        (ionbr(i,j) || 
         (!ionbr(j,i) && 
          (
           *ci<*cj || (*ci==*cj && 
           (
            // different equivalent components
            ci<cj || (!(cj<ci) /*ci==cj*/ && 
            // within same component, order according to canonical node
            // numbering,
                      ci->canonicalVertex(i)<ci->canonicalVertex(j))
            ))
           )
          )
         );
    }
  };
                                            
  // used to avoid excessive copy construction
  template <class Cmp>
  class RefCmp
  {
    const Cmp& cmp;
  public:
    RefCmp(const Cmp& cmp): cmp(cmp) {}
    bool operator()(unsigned i, unsigned j) {return cmp(i,j);}
  };

  void fillRange(OrderedCanonicalRange& result, unsigned least, unsigned most)
  {
    Range r(least,most);
    for (unsigned i=least; i<most; ++i)
      result[i]=r;
  }


  template <class Cmp>
  void sortAndFillRange(OrderedCanonicalRange& result, const Cmp& cmp)
  {
    sort(result.nodeOrder.begin(), result.nodeOrder.end(), RefCmp<Cmp>(cmp));
    unsigned least=0;  
    for (unsigned i=1; i<result.size(); ++i)
      {
        if (cmp(result.nodeOrder[least],result.nodeOrder[i])) //ie !=
          {
            if (result[least].first!=least || result[least].second!=i)
              fillRange(result,least,i);
            least=i;
          }
      }
    if (result[least].first!=least || result[least].second!=result.size())
      fillRange(result,least,result.size());
  }  

  /// iteratively apply sortAndFillRange until convergence. CmpFactory
  /// is a functional taking an OrderedCanonicalRange and returning a
  /// comparison (less than) functional operator
  template <class CmpFactory>
  void resortAndFill(OrderedCanonicalRange& o, const CmpFactory& cmpFactory)
  {
    vector<Range> last;
    do
      {
        last=o.range;
        sortAndFillRange(o, cmpFactory(o));
        assert(o.consistent());
      } while (last!=o.range);
  }

  /// A CmpFactory returning a GreaterInOutNbrhd
  class MakeGreaterInOutNbrhd
  {
    const AdjacencyMatrix& a;
  public:
    MakeGreaterInOutNbrhd(const AdjacencyMatrix& a): a(a) {}
    GreaterInOutNbrhd operator()(const OrderedCanonicalRange& o) const
    {return GreaterInOutNbrhd(a, o.range);}
  };

  /// A CmpFactory returning a GreaterInOutNbrhdWithComponent
  class MakeGreaterInOutNbrhdWithComponent
  {
    const AdjacencyMatrix& a;
    const Component& component;
  public:
    MakeGreaterInOutNbrhdWithComponent
    (const AdjacencyMatrix& a, const Component& component):
      a(a), component(component) {}
    GreaterInOutNbrhdWithComponent operator()(const OrderedCanonicalRange& o) const
    {return GreaterInOutNbrhdWithComponent(a,o.range,component);}
  };

  void CanonicalFinder::set1nodeAndResort(unsigned node, OrderedCanonicalRange& o)
  {
    unsigned min=o[node].first, max=o[node].second;
    o[node].second=min+1;
    for (unsigned j=min; j<max; ++j)
      if (j!=node) o[j].first++;
    swap(o.nodeOrder[min],o.nodeOrder[node]);
    assert(o.consistent());
    resortAndFill(o, MakeGreaterInOutNbrhd(a));
  }

  void CanonicalFinder::getInitialCanonicalRange(OrderedCanonicalRange& result)
  {
    result.range.resize(a.size(),Range(0,0));
    result.nodeOrder.resize(a.size());
    if (a.empty()) return;

    iota(result.nodeOrder.begin(),result.nodeOrder.end(),0);

    // pass 1 fill result based on node degree
    sortAndFillRange(result, GreaterDegree(g,attributes));
    resortAndFill(result, MakeGreaterInOutNbrhd(a));
  }


  inline unsigned range(const Range& x)
  {return x.second-x.first;}

  /// returns a BitRep corresponding to the relabelling in oc. Ambiguous nodes are ignored
  AttributedBitRep CanonicalFinder::bitRepOf(const OrderedCanonicalRange& oc)
  {
    AttributedBitRep r(a.size());
    for (DiGraph::const_iterator j=g.begin(); j!=g.end(); ++j)
      if (range(oc.range[j->source()])==1 && range(oc.range[j->target()])==1)
        r(oc.range[j->source()].first, oc.range[j->target()].first) = true;
    if (!attributes.empty())
      {
        r.attributes.resize(oc.size());
        for (unsigned i=0; i<oc.size(); ++i)
          if (range(oc[i])==1) 
            r.attributes[i]=attributes[oc.nodeOrder[i]];
      }
    assert(r.nodes()==g.nodes());
    return r;
  }

  double lncombinations(const vector<Range>& x)
  {
    double lnprod=0;
    for (unsigned i=0; i<x.size(); ++i) 
      lnprod+=log(range(x[i]));
    return lnprod;
  }

  /// an edge iterator that jumps edges to fixed nodes
  class NoFixedIterator
  {
    const vector<Range>& cr;
    DiGraph::const_iterator it;
    const DiGraph::const_iterator& end; ///< end of DiGraph
    bool isFixed() const ///< true if a vertex on edge is fixed
    {return it!=end && (range(cr[it->target()])==1 || range(cr[it->source()])==1);}
      
  public:
    NoFixedIterator(const vector<Range>& cr, 
                    const DiGraph::const_iterator& begin, 
                    const DiGraph::const_iterator& end):
      cr(cr), it(begin), end(end) {if (isFixed()) operator++();}
    const NoFixedIterator& operator++() {
      if (it!=end) {
        do {++it;} 
        while (isFixed());
      }
      return *this;
    }
    const Edge& operator*() const {return *it;}
#if !defined(__ICC) && defined(__GNUC__) &&  __GNUC__ > 4 || __GNUC__==4 && __GNUC_MINOR__ >= 8
    // optimisation here causes problems in gcc 4.8 (and possibly later)
#pragma GCC optimize(0)
    const Edge* operator->() const {return &*it;}
#pragma GCC reset_options
#else
    const Edge* operator->() const {return &*it;}
#endif

    //    bool operator==(const NoFixedIterator& x) const {return it==x.it;}
    bool operator!=(const NoFixedIterator& x) const {return it!=x.it;}
  };


  /** 
      component a vector of reference mapping node to the graph
      component it belongs to. This routine collects the components, and
      counts the number of each distinct component.
  */
  void collectAndCount(ComponentCount& splitGraph, Component&  component);

  template <class EdgeIterator>
  void colourGraph(ComponentCount& splitGraph, Component&  component, 
                   unsigned nodes, EdgeIterator it, const EdgeIterator end, 
                   const Attributes& attributes=Attributes())
  {
    splitGraph.clear();

    // vector of graph components, indexed by vertex
    component.clear();
    component.resize(nodes);

    for (; it!=end; ++it)
      {
        unsigned min, max;
        if (component[it->source()]->min_node()<component[it->target()]->min_node())
          {
            min=it->source();
            max=it->target();
          }
        else
          {
            max=it->source();
            min=it->target();
          }
        component[min]->push_back(*it);
        if (component[min]!=component[max])
          {
            classdesc::ref<ReducedGraph> maxGraph=component[max];
            component[min]->merge(*maxGraph);
            // set all references to maxGraph to the new merged graph
            for (unsigned i=0; i<component.size(); ++i)
              if (component[i]==maxGraph)
                component[i]=component[min];
          }
      }
    // add attributes, but don't bother if nullref and no attribute for that node
    assert(attributes.empty() || attributes.size()==nodes); 
    if (attributes.size())
      for (unsigned i=0; i<nodes; ++i)
        {
          if (!component[i].nullref() && component[i]->nodes()>1)
            component[i]->add_attributes(attributes);
          else if (attributes[i].second-attributes[i].first>1)
            component[i]->add_attributes(Attributes(1,attributes[i]));
        }
    collectAndCount(splitGraph,component);
  }

  double canonicalAttributed(AttributedBitRep& canon, UnsignedArray& remap, const Attributes& attr, const DiGraph& g)
  {
    CanonicalFinder  cf(g,attr);

    OrderedCanonicalRange oc;
    cf.getInitialCanonicalRange(oc);

    double r=cf.symmetryBreak(canon,0,oc);
    remap=oc.nodeOrder;
    return lnfactorial(g.nodes())-r;
  }

  void GraphWithC::assign(const DiGraph& g, const Attributes& attr) 
  {
    //    ecolab_nauty(g,*this,lnomega,true);
    lnomega=canonicalAttributed(*this,remap,attr,g);
    attributes.resize(attr.size());
    for (unsigned i=0; i<attr.size(); ++i)
      attributes[i]=attr[remap[i]];
  }

  unsigned MappedVertex::next=0;

  void collectAndCount(ComponentCount& splitGraph, Component&  component)
  {
    // We need to collect the unique set of components, then count the number fo each type
    typedef std::set<classdesc::ref<ReducedGraph> > UniqueComponents;
    UniqueComponents uniqueComponent(component.begin(), component.end());
    if (uniqueComponent.begin()->nullref()) 
      uniqueComponent.erase(uniqueComponent.begin());

    // list of unique components, with counts. Only do this if more than one component
    if (uniqueComponent.size()>1)
      for (UniqueComponents::iterator c=uniqueComponent.begin(); 
           c!=uniqueComponent.end(); ++c)
        {
          //cout << "adding component"<<(*c)->graph()<<endl;
          splitGraph[(*c)->graph()]++;
        }

    assert(splitGraph.size()!=1||splitGraph.begin()->second>1);
    // initialise all the null refs
    for (unsigned i=0; i<component.size(); ++i)
      *component[i]; 
  }

  /**
     remove fixed nodes from graph, and colour the rest according to
     connectivity. Split range according to colour. If range does not
     split, return false.
  */
  bool CanonicalFinder::starExplode
  (double& r, AttributedBitRep& result, unsigned pos, 
   OrderedCanonicalRange& o)
  {
    using classdesc::ref;

    NoFixedIterator nfi(o.range,g.begin(),g.end()), 
      nfend(o.range,g.end(),g.end());

    // attach the set of fixed neighbours as attributes to the nodes.
    //Attributes fixedNeighbours(o.range);
    ComponentCount componentCount;
    Component component;    
    colourGraph(componentCount,component,g.nodes(),nfi,nfend,o.range);

    // if we have more than one component in our graph
    if (componentCount.size())
      {
        for (ComponentCount::iterator i=componentCount.begin();
             i!=componentCount.end(); ++i)
          r+=lnfactorial(i->second) + i->second *
            (lnfactorial(i->first.nodes()) - i->first.lnomega);

        // renumber nodes according to the components
        resortAndFill(o, MakeGreaterInOutNbrhdWithComponent(a,component));
        result=bitRepOf(o);

        assert(o.consistent());
        assert(unique(o.range));
        assert(unique(o.nodeOrder));
        assert(result.nodes()==g.nodes());
        assert(result.links()==g.links());
        return true;
      }
    else
      return false;
  }

  double CanonicalFinder::symmetryBreak
  (AttributedBitRep& result, unsigned pos, OrderedCanonicalRange& range)
  {
    double r=0;
    const UnsignedArray& n=range.nodeOrder;

    double comb=lncombinations(range.range);
    if (comb==0) 
      {
        // in this case, we have a complete canonical labelling
        result=bitRepOf(range);
        assert(result.nodes()==g.nodes());
        assert(result.links()==g.links());
        return r;
      }

    // according to timing tests, doing starExplode everytime works best!
    //    if (comb>1)
    {
      if (starExplode(r, result, pos, range))  
        return r;
    }

    // now apply the symmetry breaker algorithm 
    for (unsigned i=pos; i<n.size(); ++i)
      {
        unsigned min=range[i].first, max=range[i].second;
        if (max-min>1)
          {
            unsigned j;
            map<AttributedBitRep, vector<OrderedCanonicalRange> > rangeNodes;

            // First, break the symmetry by fixing one node, then
            // store the resulting ranges in canonical order
            for (j=min; j<max; ++j)
              {
                OrderedCanonicalRange tmpC(range);
                set1nodeAndResort(j,tmpC);
                rangeNodes[bitRepOf(tmpC)].push_back(tmpC);
              }

            // pick the list of ranges having smallest canonical order
            vector<OrderedCanonicalRange>& crange=rangeNodes.begin()->second;

            // this map stores the log of the automorphism group size
            map<AttributedBitRep, vector<double> > rangeValue;
            // this map stores the OrderCanonicalRange
            map<AttributedBitRep, OrderedCanonicalRange > ocRange;
            for (j=0; j<crange.size(); ++j)
              {
                OrderedCanonicalRange tmpC(crange[j]);
                AttributedBitRep rep;
                double r=symmetryBreak(rep,i+1,tmpC);
                assert(rep.nodes()==g.nodes());
                assert(rep.links()==g.links());
                rangeValue[rep].push_back(r);
                ocRange[rep].swap(tmpC);
                // assert that automorphism groups sizes are consistent
                assert(fabs(r-rangeValue[rep][0])<log(1.5));
              }
            result=rangeValue.begin()->first;
            range.swap(ocRange.begin()->second);
            r+= log(rangeValue.begin()->second.size());
            return rangeValue.begin()->second[0] + r;
          }
      }
    return r;
  }



  double canonical(BitRep& canon, UnsignedArray& remap, const DiGraph& g)
  {
    AttributedBitRep aCanon;
    double r=canonicalAttributed(aCanon,remap,Attributes(),g);
    canon=aCanon;
    return r;
  }

}

namespace ecolab
{
  double canonical(BitRep& canon, const Graph& g)
  {
    UnsignedArray remap;
    DiGraph gg(g);
    return ::canonical(canon,remap,gg);
  }
}

