/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graph.h"
#include "ecolab_epilogue.h"

#include <ctype.h>
#include <fstream>
using namespace ecolab;
using namespace std;

namespace 
{

  void output_pajek(const Graph& g, ostream& o)
  {
  
    o << "*Vertices " <<g.nodes()<< "\r\n";
    for (size_t i=0; i<g.nodes(); i++)
      o << i+1 << " " << i+1 << "\r\n";

    if (g.directed())
      {
        o << "*Arcs\r\n";
      
        for (Graph::const_iterator j=g.begin(); j!=g.end(); ++j)
          o << j->source()+1 << " " << j->target()+1 << " "<<
            j->weight<<"\r\n";
      }
    else
      {
        o << "*Edges\r\n";

        // skip reverse edge in outputs, as it is implied
        for (Graph::const_iterator j=g.begin(); j!=g.end(); ++j, ++j)
          o << j->source()+1 << " " << j->target()+1 << " "<<j->weight<<"\r\n";
      }
  }

  void input_pajek(Graph& g, istream& i)
  {
    std::string buf, buf2;
    Edge e;
    const std::string Arcs="*arcs\r", Edges="*edges\r";
    // look for *Arcs keyword
    while (getline(i,buf))
      {
        {
          for (unsigned i=0; i<buf.length(); ++i)
            buf[i]=tolower(buf[i]);
        }
        if (buf==Arcs || buf==Edges)
          while (getline(i,buf2))
            {
              if (buf2[0]!='*')
                {
                  istringstream is(buf2);
                  is>>e.source()>>e.target();
                  if (e.source()==e.target()) continue; //ignore self edges
                  if (!(is>>e.weight)) e.weight=1;
                  //                  cout << e.source() << " " << e.target() << " "<<e.weight<<endl;
                  e.source()--; e.target()--;
                  g.push_back(e);
                  if (buf==Edges)
                    g.push_back(Edge(e.target(),e.source(),e.weight));
                }
              else
                break;
            }
      }
  }

  // output dense matrix form (suitable for Matlab/Octave use
  void output_mat(const Graph& g, ostream& o)
  {
    vector<double> m(g.nodes()*g.nodes());
    for (Graph::const_iterator j=g.begin(); j!=g.end(); ++j)
      m[j->target()*g.nodes()+j->source()] = j->weight;

    for (size_t i=0, p=0; i<g.nodes(); ++i)
      {
        for (size_t j=0; j<g.nodes(); ++j, ++p)
          o << m[p] << " ";
        o << endl;
      }
  }

  void input_mat(Graph& g, istream& i)
  {
    size_t nodes=0, rowsRead=0;
    double val;
    while (i || nodes==0 || rowsRead<nodes)
      {
        string line;
        getline(i,line);
        istringstream ss(line);
        vector<double> row;
        while (ss>>val) row.push_back(val);
        if (nodes<row.size()) nodes=row.size(); 
        for (size_t j=0; j<row.size(); ++j)
          if (row[j]!=0)
            g.push_back(Edge(j, rowsRead, row[j]));
        rowsRead++;
      }
  }

  void output_LGL(const Graph& g, ostream& o)
  {
    // construct a map of connected neighbours of each node. Ensure
    // uniques of edges, as LGL is particular about graphs being
    // undirected
    vector<map<unsigned, float> > neighbourlist(g.nodes());
    for (Graph::const_iterator i=g.begin(); i!=g.end(); ++i)
      {
        unsigned min_node, max_node;
        if (i->source()<i->target())
          {min_node=i->source(); max_node=i->target();}
        else
          {max_node=i->source(); min_node=i->target();}
        neighbourlist[min_node][max_node+1]=i->weight;
      }
    for (unsigned i=0; i<g.nodes(); ++i)
      {
        o << "# "<<(i+1)<<endl;
        for (map<unsigned,float>::const_iterator j=neighbourlist[i].begin(); 
             j!=neighbourlist[i].end(); ++j)
          {
            o << j->first;
            if (j->second != 1.0)
              o << " " << j->second; // include optional weight
            o << endl;
          }
      }
  }

  class NodeMapperID
  {
    unsigned id;
  public:
    static unsigned next_nodeID;
    NodeMapperID(): id(next_nodeID++) {}
    operator unsigned() const {return id;}
  };

  unsigned NodeMapperID::next_nodeID=0;

  void input_LGL(Graph& g, istream& i)
  {
    std::string buf;
    // LGL files use node labels (assumed not to contain whitespace),
    // so we need to allocated unsigned int labels as each one is encountered
    NodeMapperID::next_nodeID=0; //reset numeric labels to go from zero
    map<std::string,NodeMapperID> nodeID;

    unsigned node1;
    while (getline(i,buf))
      {
        istringstream is(buf);
        if (buf[0]=='#')
          {
            char c;
            std::string label;
            is >> c >> label;
            node1=nodeID[label];
          }
        else if (!nodeID.empty()) // ensure node1 is valid
          {
            std::string node2label;
            if (is >> node2label) //ignore blank lines
              {
                float w=1;
                is >> w; // try to read a weight
                unsigned node2=nodeID[node2label];
                g.push_back(Edge(node1,node2,w));
                g.push_back(Edge(node2,node1,w)); // LGL files are always undirected
              }
          }
      }
  }

  /*
    The format used by the gengraph software by Fabien Viger and Matthieu Latapy
  */
  void input_gengraph(Graph& g, istream& i)
  {
    std::string buf;
    while (getline(i,buf))
      {
        unsigned node1, node2;
        istringstream is(buf);
        is >> node1;
        while (is >> node2)
          g.push_back(Edge(node1,node2));
      }
  }
}

namespace ecolab
{

  void Graph::output(const string& format, const string& filename) const
  {
    ofstream o(filename.c_str());
    if (format=="pajek") output_pajek(*this,o);
    if (format=="lgl") output_LGL(*this,o);
    if (format=="dot") o<<*this;
    if (format=="mat") output_mat(*this,o);
  }

  void Graph::input(const string& format, const string& filename)
  {
    ifstream i(filename.c_str());
    if (format=="pajek") input_pajek(*this,i);
    if (format=="lgl") input_LGL(*this,i);
    if (format=="dot") i>>*this;
    if (format=="gengraph") input_gengraph(*this,i);
    if (format=="mat") input_mat(*this,i);
  }

  namespace 
  {
    template <class G>
    void parse_graphviz(istream& s, G& x, bool bidir)
    {
      std::string buf;
      x.clear();
      while (getline(s,buf))
        {
          unsigned node1, node2;
          sscanf(buf.c_str(),"%d%*2c%d",&node1,&node2);
          // attempt to read weight attribute
          float weight = 1;
          const char* c=strstr(buf.c_str(), "weight");
          if (c)
            {
              c+=sizeof("weight");
              // find and skip '='
              c = strchr(c,'=');
              if (c)
                {
                  c++;
                  while (isspace(*c)) c++; 
                  if (isdigit(*c)||*c=='.'||*c=='-')
                    weight=atof(c);
                }
            }
          x.push_back(Edge(node1,node2,weight));
          if (bidir) x.push_back(Edge(node2,node1,weight));
        }
    }

    template <class T, class G>
    bool asg(member_entry_base* me, const G& y)
    {
      // should really do this via a virtual function!
      member_entry<T>* r=*dynamic_cast<member_entry<T>*>(me);
      if (r)
        r->memberptr=y;
      return r;
    }

    template <class G> istream& insert_impl(istream& s, G& x)
    {
      // check to see if data is passed inline, or it is just a reference
      std::string buf;
      getline(s,buf);
      if (buf=="graph {") 
        parse_graphviz(s,x,true);
      else if (buf=="digraph {") 
        parse_graphviz(s,x,false);
      else
        {
          if (TCL_obj_properties().count(buf)==0)      
            throw error("%s does not exist!",buf.c_str());
          if (Graph* g=TCL_obj_properties()[buf]->memberPtrCasted<Graph>())
            x=*g;
          else
            throw error("unable to assign variable %s, which is not a Graph, "
                        "to object of type %s", buf.c_str(), typeid(G).name());
        }
      return s;
    }
  }
                            
  void ErdosRenyi_generate(Graph& g, unsigned nodes, unsigned links, 
                           urand& uni, random_gen& weight_dist)
  {
    g.clear(nodes);
    assert(links<=nodes*(nodes-1));
    while (g.links()<links)
      {
        unsigned node1=nodes*uni.rand(), node2;
        do
          node2=nodes*uni.rand();
        while (node1==node2);
        g.push_back(Edge(node1,node2,weight_dist.rand()));
      }
  }

  void PreferentialAttach_generate(Graph& g, unsigned nodes, urand& uni,
                                   random_gen& indegree_dist, random_gen& weight_dist)
  {
    g.clear();
    vector<unsigned> degree(nodes);
    unsigned totalDegree=2;
    // insert a single edge to start the algorithm
    degree[0]=degree[1]=1;
    g.push_back(Edge(0,1,weight_dist.rand()));


    for (unsigned node=2; node<nodes; ++node)
      {
        unsigned indegree;
        //we need to attach
        while ( (indegree=indegree_dist.rand()+0.5) <1);
  
        for (unsigned j=0; j<indegree && j<g.nodes(); ++j)
          {
            unsigned attach_to;
            while ((attach_to=degree.size()*uni.rand())==node || 
                   (uni.rand()*totalDegree >= degree[attach_to]
                    && !g.contains(Edge(attach_to,node)))
                   );
            g.push_back(Edge(attach_to,node,weight_dist.rand()));
            ++degree[attach_to];
            ++degree[node];
            totalDegree+=2;
          }
      }
  }

  void random_rewire(Graph& g, urand& u)
  {
    ConcreteGraph<DiGraph> src(g);
    g.clear(src.nodes());
    unsigned s,t;
    Edge e1;

    ConcreteGraph<DiGraph>::const_iterator e=src.begin();
    for (; e!=src.end(); ++e)
      {
        do
          {
            s=u.rand()*g.nodes(); 
            t=u.rand()*g.nodes();
          }
        while (s==t || g.contains(e1=Edge(s,t,e->weight)));
        g.push_back(e1);
      }
  }

}

std::ostream& std::operator<<(std::ostream& s, const Graph& x)
{
  if (x.directed())
    {
      s << "digraph {\n";
      for (Graph::const_iterator i=x.begin(); i!=x.end(); ++i)
        {
          s << i->source()<<"->"<<i->target();
          if (i->weight != 1.0)
            s << "[weight = "<<i->weight<<"]";
          s <<";\n";
        }
      s<<"}\n";
    }
  else
    {
      s << "graph {\n";
      // increment iterator twice, as links are repeated in reverse order
      for (Graph::const_iterator i=x.begin(); i!=x.end(); ++i,++i)
        {
          s << i->source()<<"--"<<i->target();
          if (i->weight != 1.0)
            s << "[weight = "<<i->weight<<"]";
          s <<";\n";
        }
      s<<"}\n";
    }
  return s;
}

istream& std::operator>>(istream& s, Graph& x) {return insert_impl(s,x);}

