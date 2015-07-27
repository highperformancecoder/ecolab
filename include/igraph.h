/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**
  An EcoLab graph adaptor for igraph objects
*/

#ifndef IGRAPH_H
#define IGRAPH_H
#include "graph.h"
#include "igraph/igraph.h"


// TODO fix up error handling
namespace ecolab
{
  struct IGraphError: public std::exception
  {
    int err;
    IGraphError(int err): err(err) {}
    const char* what() const throw()
    {return igraph_strerror(err);}
  };

  class IGraph: public igraph_t
  {
    static int errorhandler;
    void addEdges(const Graph& g);
  public:
    /// construct an iGraph with \a nodes, \a directed or not
    IGraph(unsigned nodes=0, bool directed=false) 
    {igraph_empty(this, nodes, directed);}
    IGraph(const IGraph& x) {igraph_copy(this,&x);}
    IGraph(const Graph& x) {
      igraph_empty(this, x.nodes(), x.directed());
      addEdges(x);
    }
    ~IGraph() {
      ++errorhandler; //does nothing other than ensure linkage
      try {igraph_destroy(this);}
      catch (std::exception& ex) {
        std::cerr << "unexpected exception encountered: "<<
          ex.what()<<std::endl;
      } 
    }
    

    const IGraph& operator=(const IGraph& x) 
    {igraph_copy(this,&x); return *this;}
    const IGraph& operator=(const Graph& x) {
      clear(x.nodes());
      addEdges(x);
      return *this;
    }

    class const_iterator
    {
      const igraph_t* g;
      igraph_eit_t it;
      mutable Edge edge;
    public:
      const_iterator(const igraph_t& graph, const igraph_es_t& es): 
        g(&graph) {igraph_eit_create(g,es,&it);}
      ~const_iterator() {igraph_eit_destroy(&it);}
      bool operator==(const const_iterator& x) const {
        // return true if both iterators are end, otherwise if they're equal
        return (g==x.g && 
                IGRAPH_EIT_END(it) && IGRAPH_EIT_END(x.it)) || 
          (!IGRAPH_EIT_END(it) && !IGRAPH_EIT_END(x.it) && operator*()==*x);
      }
      bool operator!=(const const_iterator& x) const {return !operator==(x);}
      Edge operator*() const {
        igraph_integer_t from, to;
        igraph_edge(g, IGRAPH_EIT_GET(it), &from, &to);
        Edge e(from,to);
        return e;
      }
      const Edge* operator->() const {
        edge=operator*();
        return &edge;
      }
      const_iterator& operator++() {
        IGRAPH_EIT_NEXT(it);
        return *this;
      }
    };
  
    const_iterator begin() const
    {
      igraph_es_t es;
      igraph_es_all(&es,IGRAPH_EDGEORDER_ID); //select all edges
      return const_iterator(*this,es);
    }

  
    const_iterator end() const
    {
      igraph_es_t es;
      igraph_es_none(&es); //select no edges
      return const_iterator(*this,es);
    }

    unsigned nodes() const {return igraph_vcount(this);}
    unsigned links() const {return igraph_ecount(this);}
    void push_back(const Edge& e) 
    {
      unsigned n=nodes(), maxN=std::max(e.source(), e.target())+1;
      if (n<maxN) igraph_add_vertices(this,maxN-n,0); //ensure enough nodes
      igraph_add_edge(this, e.source(), e.target());
    }

    bool contains(const Edge& e) const {
      try {
        igraph_integer_t eid;
        igraph_get_eid(this,&eid,e.source(),e.target(),true);
        return true; // edge exists if eid found
      }
      catch (std::exception) {return false;}
    }

    bool directed() const {return igraph_is_directed(this);}

    void clear(unsigned nodes=0) {
      bool dir=directed();
      igraph_destroy(this);
      igraph_empty(this,nodes,dir);
    } 
  };
    
}
#endif
