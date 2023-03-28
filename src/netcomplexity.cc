/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "netcomplexity.h"

#ifdef IGRAPH
#include "igraph.h"
#endif

#include "ecolab_epilogue.h"
#include "nauty.h"
#include "nautinv.h"
#include <math.h>
#include <algorithm>
#include <limits>
#include <fstream>
#include <signal.h>
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
#include <sys/wait.h>
#endif

using namespace std;

namespace ecolab
{
  //typedef vector<bool> bvec;
  //typedef vector<int> bvec;
  typedef bitvect bvec;

  void setvec(bitvect& x,const bitvect& y)
  {x=y;}

  void bitvect::setrange(unsigned start, unsigned end, bool value)
  {
    if (start==end) return;
    assert(end<=sz);
    assert(div(start)<nwords() && div(end-1)<nwords());
    unsigned me = mod(end-1)+1; /* this is such a kludge */
    unsigned ds=div(start), de=div(end-1);
    if (value)
      if (ds==de)
        data[ds]|=bitrange(mod(start))&~bitrange(me);
      else
        {
          data[ds]|=bitrange(mod(start));
          memset(data.data()+ds+1,255,(de-ds-1)*(WORDSIZE>>3));
          data[de]|=~bitrange(me);
        }
    else
      if (ds==de)
        data[ds]&=~bitrange(mod(start))|bitrange(me);
      else
        {
          data[ds]&=~bitrange(mod(start));
          memset(data.data()+ds+1,0,(de-ds-1)*(WORDSIZE>>3));
          data[de]&=bitrange(me);
        }
  }    

  unsigned bitvect::popcnt(unsigned start, int end) const 
  {
    if (sz==0) return 0;
    if (end<0) end=sz;
    assert(div(start)<nwords());
    assert(div(end-1)<nwords());
    unsigned me=mod(end-1)+1;
    if (div(start)==div(end-1))
      return POPCOUNT(data[div(start)]&bitrange(mod(start))&~bitrange(me));
    else
      {
        unsigned i, r=POPCOUNT(data[div(start)]&bitrange(mod(start)));
        for (i=div(start)+1; i<div(end-1); i++) r+=POPCOUNT(data[i]);
        r+=POPCOUNT(data[div(end-1)]&~bitrange(me));
        return r;
      }
  }

  /* increment the binary sequence */
  void bitvect::operator++(int)
  {
    int i;
    for (i=size()-1; i>=0 && (*this)[i]; i--)
      (*this)[i]=false;
    if (i>=0) (*this)[i]=true;
  }

  /* next permutation sequence (nos of zeros & ones kept constant). 
     returns true if set to next perumtation */
  bool bitvect::next_perm()
  {
    unsigned i, first1, next0;
    for (first1=0; !(*this)[first1]; first1++) /* find first one */
      if (first1==size()-1) {return false;} /* all 0s, no next sequence */
    for (next0=first1; (*this)[next0]; next0++)
      if (next0==size()-1) {return false;} /* remainder is all 1s, no next sequence*/
    for (i=0; i<next0-first1-1; i++) 
      (*this)[i]=true; /* shuffle 1s back */
    for (; i<next0; i++) (*this)[i]=false;
    (*this)[next0]=true;             /* move leading 1 up one pos. */
    return true;
  }

  bool symmetric(const BitRep& x)
  {
    bool r=true;
    for (unsigned i=1; i<x.nodes(); i++)
      for (unsigned j=0; j<i;j++)
        r&=x(i,j)==x(j,i);
    return r;
  }

  template <class T>
  vector<T> permute(const vector<T>& x, const vector<unsigned>& perm)
  {
    vector<T> r(x.size());
    for (int i=0; i<x.size(); i++) r[i]=x[perm[i]];
    return r;
  }

  double lnfactorial(unsigned x)
  {
    /* use simple multiplication for small x */
    if (x<10)
      {
        unsigned r=1;
        for (unsigned i=2; i<x+1; i++)
          r*=i;
        return log(double(r));
      }
    else
      return (x+0.5)*log(double(x+1))-x-0.081061467; /* Abramowitz & Stegun 6.1.41 */
  }

  double lnCombine(unsigned x, unsigned y)
  {
    assert(x>=y);
    unsigned xmy=x-y;
    // make xmy >= y
    if (y>xmy) std::swap(y,xmy);
    if (y<10)
      {
        if (x==xmy) return 0;
        double r=x;
        for (unsigned i=1; i<y; ++i)
          r*=double(x-i)/double(i+1);
        return log(r);
      }
    else
      return lnfactorial(x)-lnfactorial(x-y)-lnfactorial(y);
  }

  double lnsum(double x,double y)
  {
    if (x>15*y) return x;
    if (y>15*x) return y;
    return log(1+exp(x/y))+y;
  }
 
  void ecolab_nauty(const NautyRep& g, NautyRep& canonical, double& lnomega, 
                    bool do_canonical, int invariant_option)
  {
    DEFAULTOPTIONS(options);
    options.getcanon=do_canonical;
    options.digraph=true;

    statsblk status;
    const int n=g.nodes(), m=SETWD(n-1)+1;
    int orbits[n], lab[n], ptn[n], worksize=50*m+1;
    setword workspace[worksize];

    switch (invariant_option)
      {
      case 0:
        break; //do nothing, default
      case 1:
        options.invarproc=twopaths;
        break;
      case 2:
        options.invarproc=adjtriang;
        options.invararg=0;
        break;
      case 3:
        options.invarproc=adjtriang;
        options.invararg=1;
        break;
      case 4:
        options.invarproc=triples;
        break;
      case 5:
        options.invarproc=quadruples;
        break;
      case 6:
        options.invarproc=celltrips;
        break;
      case 7:
        options.invarproc=cellquads;
        break;
      case 8:
        options.invarproc=cellquins;
        break;
      case 9:
        options.invarproc=distances;
        options.invararg=0;
        break;
      case 10:
        options.invarproc=indsets;
        options.invararg=5;
        break;
       case 11:
        options.invarproc=cliques;
        options.invararg=5;
        break;
       case 12:
        options.invarproc=cellcliq;
        options.invararg=5;
        break;
       case 13:
        options.invarproc=cellind;
        options.invararg=5;
        break;
       case 14:
        options.invarproc=adjacencies;
        options.maxinvarlevel=10;
        break;
       case 15:
        options.invarproc=cellfano;
        break;
      case 16:
        options.invarproc=cellfano2;
        break;
      }

    if (do_canonical) canonical=NautyRep(n);
    nauty(const_cast<graph*>(&g.linklist.data[0]),lab,ptn,NULL,orbits,&options,
          &status,workspace,worksize,m,n,
          do_canonical? &canonical.linklist.data[0]: NULL);
    lnomega=lnfactorial(n) - log(status.grpsize1) - log(10)*status.grpsize2;
  }

  double NautyRep::lnomega() const
  {
    double lnomega;
    NautyRep dummy;
    ecolab_nauty(*this,dummy,lnomega,false);
    return lnomega;
  }

  NautyRep NautyRep::canonicalise() const
  {
    NautyRep canonical;
    double lnomega;
    ecolab_nauty(*this,canonical,lnomega,true);
    return canonical;
  }

#ifdef SAUCY
  extern "C"
  {
#include <saucy.h>
  }

  int saucy_null_consumer(int, const int *, int, int *, void *) {return 0;}

  class SaucyRep
  {
    saucy_graph rep;
    vector<int> adj, edg;
  public:
    // why twice? it appears Saucy uses this undocumented area as workspace.
    SaucyRep(const DiGraph& g): adj(2*g.nodes()+2), edg(2*g.links())
    {
      rep.n=g.nodes();
      rep.e=g.links();
      rep.adj=adj.data();
      rep.edg=edg.data();
      // count neighbours of each node
      for (DiGraph::const_iterator i=g.begin(); i!=g.end(); ++i)
        if (i->source()<g.nodes()-1)
          adj[i->source()+2]++;
      // perform running sum
      for (size_t i=3; i<adj.size(); ++i)
        adj[i]+=adj[i-1];
      // now fill in edges
      for (DiGraph::const_iterator i=g.begin(); i!=g.end(); ++i)
        edg[adj[i->source()+1]++]=i->target();
    }
    double lnOmega()
    {
      saucy* s=saucy_alloc(rep.n);
      saucy_stats stats;
      vector<int> colours(rep.n);
      saucy_search(s,&rep,true,colours.data(),saucy_null_consumer,NULL,&stats);
      saucy_free(s);
      return lnfactorial(rep.n) - stats.grpsize_exp * log(10) - 
        log(stats.grpsize_base);
    }
  };

  double saucy_lnomega(const DiGraph& g) {return SaucyRep(g).lnOmega();}
#else
  // dummy it with a call to Nauty
  double saucy_lnomega(const DiGraph& g) {return NautyRep(g).lnomega();}
#endif

#ifdef IGRAPH
  double igraph_lnomega(const DiGraph& g, int method)
  {
    GraphAdaptor<const DiGraph> ga(g);
    IGraph ig(ga);
    igraph_bliss_sh_t sh(static_cast<igraph_bliss_sh_t>(method));
    igraph_bliss_info_t ret;
    igraph_automorphisms(&ig,sh,&ret);
    double r;
    istringstream is(ret.group_size);
    is>>r;
    free(ret.group_size);
    return lnfactorial(g.nodes())-log(r);
  }
#else
  // dummy it with a call to Nauty
  double igraph_lnomega(const DiGraph& g, int method) {return NautyRep(g).lnomega();}
#endif  

  double hybridLnOmega(const DiGraph& g)
  {
    // Mingw simply does not support this sort of process control
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
    // set up return pipe
    const int read_pipe=0, write_pipe=1;
    int pipes[2];
    if (pipe(pipes)!=0)
      throw error(strerror(errno));

    // now fork child processes to try Nauty and SuperNOVA simultaneously
    vector<pid_t> children;
    children.push_back(fork());
    if (children.back()==0)
      {
        // child process
        double r=NautyRep(g).lnomega(); // Nauty version

        // TODO: not sure that throwing an exception in these
        // circumstances helps much
        if (write(pipes[write_pipe], &r, sizeof(r))==-1)
          throw error(strerror(errno));
 
        int w=0;
        if (write(pipes[write_pipe], &w, sizeof(w))==-1)
          throw error(strerror(errno));

        close(pipes[write_pipe]);
        close(pipes[0]); close(pipes[1]);
        exit(0);
      }
    else if (children.back() == -1)
      throw error(strerror(errno));

    children.push_back(fork());
    if (children.back()==0)
      {
        BitRep canon;
        double r=canonical(canon,g); //SuperNOVA version
        if (write(pipes[write_pipe], &r, sizeof(r))==-1)
          throw error(strerror(errno));
        int w=1;
        if (write(pipes[write_pipe], &w, sizeof(w))==-1)
          throw error(strerror(errno));
        close(pipes[0]); close(pipes[1]);
        exit(0);
      }
    else if (children.back() == -1)
      throw error(strerror(errno));

    // Bliss (from igraph) produces errors occasionally. Until this is
    // resolved, disable from the complexity calculation
//#ifdef IGRAPH
//    // methods 0 & 2 work best
//    for (int method=0; method<3; method+=2)
//      {
//        children.push_back(fork());
//        if (children.back()==0)
//          {
//            double r=igraph_lnomega(g, method);
//            if (write(pipes[write_pipe], &r, sizeof(r))==-1)
//              throw error(strerror(errno));
//            int w=1;
//            if (write(pipes[write_pipe], &w, sizeof(w))==-1)
//              throw error(strerror(errno));
//            close(pipes[0]); close(pipes[1]);
//            exit(0);
//          }
//        else if (children.back() == -1)
//          throw error(strerror(errno));
//      }
//#endif

    double r;
    int winner;
    if (read(pipes[read_pipe],&r,sizeof(r))==-1 ||
        read(pipes[read_pipe],&winner,sizeof(winner))==-1)
      throw error(strerror(errno));
    for (size_t i=0; i<children.size(); ++i)
      {
        //      cout << "killing pid: "<<children[i]<<endl;
        kill(children[i], SIGKILL); // kill off the slower process
      }
    //needed to release child process resources
    for (size_t i=0; i<children.size(); ++i)
      waitpid(children[i], NULL, 0);
    close(pipes[0]); close(pipes[1]);
    //  ofstream winf("winners.dat",ios_base::app);
    //  winf << g.nodes() <<" "<<g.links()<<" "<<winner<<endl;

    return r;
#else // on MINGW, just call Nauty
    return NautyRep(g).lnomega();
#endif
  }

  // a structure for computing the complexity of a partial graph
  struct PG
  {
    DiGraph partialG;
    double weight;
    double baseComplexity;
    PG(const DiGraph& p, double w, double b): partialG(p), weight(w), baseComplexity(b) {}
    double partialComplexity(double C)
    {
      // compute the complexity of this term, if it is going to
      // contribute significantly to the total
      double err=weight*baseComplexity;
      //    cout << "w= " << weight<<", bc="<< baseComplexity<<" err/C="<<(err/C) <<
      //      " l/n="<<(double(partialG.links())/partialG.nodes()) <<endl;
      if (err<1E-3 * C)
        return err;
      else
        return weight * (baseComplexity-hybridLnOmega(partialG)/log(2));
      //      if (partialG.links() < 3*partialG.nodes()) //use SuperNOVA
      //        {
      //          BitRep canon;
      //          return weight * (baseComplexity-canonical(canon,partialG)/log(2));
      //        }
      //      else
      //        return weight * (baseComplexity-NautyRep(partialG).lnomega()/log(2));
    }
  };



  double complexity(const Graph& g)
  {
    /* create a multimap of edges, stored in inverse weight order */
    typedef std::multimap<float, Edge, std::greater<float> > OrderedEdges;
    OrderedEdges orderedEdges;
    for (Graph::const_iterator e=g.begin(); e!=g.end(); ++e)
      {
        orderedEdges.insert(std::make_pair(e->weight,*e));
      }

    double emptyComplexity=baseComplexity(g.nodes(),0,!g.directed());
    if (g.links()==0) return emptyComplexity;

    DiGraph partialG(g.nodes());
    float sumWeight=0;
    double c=0;

    vector<PG> pgv;

    for (OrderedEdges::const_iterator e=orderedEdges.begin(); e!=orderedEdges.end(); )
      {
        float w=e->first;
        sumWeight+=w;
        // fill the partial graph with links of the same edgeweight
        for (; e!=orderedEdges.end() && w==e->first; ++e)
          {
            assert(!partialG.contains(e->second));
            partialG.push_back(e->second);
          }

        pgv.push_back(PG(partialG, w, baseComplexity(g.nodes(),partialG.links(),!g.directed())));
      }

    // now find the term with the maximum weighted base complexity
    size_t maxi=0;
    double maxwbc=0, wbc;
    for (size_t i=0; i<pgv.size(); ++i)
      if ((wbc=pgv[i].weight * pgv[i].baseComplexity) > maxwbc)
        {
          maxwbc = wbc;
          maxi=i;
        }

    size_t M=2*max(pgv.size()-maxi, maxi);
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
    for (size_t i=0; i<M; ++i)
      {
        double lc=0, cc=c;
        if (i%2==0 && maxi+i/2 < pgv.size())
          lc=pgv[maxi+i/2].partialComplexity(cc);
        if (i%2==1 && maxi>i/2)
          lc=pgv[maxi-i/2-1].partialComplexity(cc);
        if (lc>0)
          {
#ifdef _OPENMP
#pragma omp atomic
#endif
            c+=lc;
          }
        //      cout << " " << (c/sumWeight) << endl;
      }
    c/=sumWeight;
    return c;
  }

  ostream& operator<<(ostream& s, const bitvect& x)
  { 
    for (unsigned i=0; i!=x.size(); i++)
      s << bool(x[i]);
    return s;
  }

  double ODcomplexity(const DiGraph& x)
  {
    vector<unsigned> l(x.nodes());
    double oneonL=2.0/x.size(), r=0;

    DiGraph::const_iterator i;
    for (i=x.begin(); i!=x.end(); i++)
      l[i->source()]++;
    unsigned maxl=x.size()>0? *std::max_element(l.begin(),l.end())+1: 0;
    vector<vector<double> > c(maxl,vector<double>(maxl));
    for (i=x.begin(); i!=x.end(); i++)
      {
        if (l[i->source()]>l[i->target()])
          c[l[i->target()]][l[i->source()]]+=oneonL;
        else if (l[i->source()]==l[i->target()])
          c[l[i->target()]][l[i->source()]]+=0.5*oneonL; /*allow for double counting*/
      }
    for (unsigned k=0; k<maxl; k++)
      {
        double a;
        //      for (unsigned i=0; i<maxl-k; i++)
        //	a+=c[i][k+i];
        for (unsigned i=0; i<=k; i++)
          {
            a=c[i][k];
            if (a>0)
              r-=a*log(a);
          }
      }
    return r;
  }

  double MA(const DiGraph& x)
  {
    // Firstly find the row & column sums of weights
    vector<double> rowSum(x.nodes()), colSum(x.nodes());
    double W=0; //total weight
    for (DiGraph::iterator l = x.begin(); l != x.end(); ++l)
      {
        rowSum[l->source()] += l->weight;
        colSum[l->target()] += l->weight;
        W += l->weight;
      }

    // now form the redundancy R and mututal information I
    double R=0, I=0;
    for (DiGraph::iterator l = x.begin(); l != x.end(); ++l)
      {
        double w = l->weight / W;
        double denom = rowSum[l->source()] * colSum[l->target()];
        R += w * log(l->weight*l->weight / denom);
        I += w * log(l->weight * W / denom);
      }
    
    return -R*I / log(2.0); // in bits
    
  }

}
