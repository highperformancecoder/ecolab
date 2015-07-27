/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <classdesc_access.h>
#include <pack_stl.h>
#include <analysis.h>

#if defined(GNUSL)
#include <gsl/gsl_errno.h>
#include <gsl/gsl_odeiv2.h>
#endif

using namespace ecolab;
typedef map<NautyRep,double> archetype;
using namespace classdesc;
using array_ns::array;

struct Gsl_matrix;
struct GaussianMarkovModel
{
  /// lags and replications used calculating causal density and ilk
  int lags;
  Gsl_matrix* runData;
  typedef ConcreteGraph<DiGraph> G;
  G& g;
  gaussrand gran;

  GaussianMarkovModel(G& g): lags(10), runData(NULL), g(g) {}
  ~GaussianMarkovModel();
  /// run the model for lags+repetitions steps to populate the matrices
  void run(TCL_args repetitions);
  /// output the rundata into a file: each node has separate row, each
  /// sample column
  void outputRunData(TCL_args);
  /// calculate the causal density for a Gaussian Markov process
  /// running on the network.
  double causalDensity();
  double bipartitionCausalDensity();
};

// RAII data structure for GSL Runge-Kutta algorithm
struct RKdata
{
#if defined(GNUSL)
  gsl_odeiv2_system sys;
  gsl_odeiv2_driver* driver;
  template <class DynSysGen>
  RKdata(const DynSysGen& dynSys) {
    sys.function=DynSysGen::RKfunction;
    sys.dimension=dynSys.dim;
    sys.params=(void*)&dynSys;
    const gsl_odeiv2_step_type* stepper=gsl_odeiv2_step_rkf45;
    driver = gsl_odeiv2_driver_alloc_y_new
        (&sys, stepper, dynSys.stepMax, dynSys.epsAbs, 
         dynSys.epsRel);
      gsl_odeiv2_driver_set_hmax(driver, dynSys.stepMax);
      gsl_odeiv2_driver_set_hmin(driver, dynSys.stepMin);
    }
    ~RKdata() {gsl_odeiv2_driver_free(driver);}
#else
  int sys, driver; // dummies, to keep classdesc happy
#endif
  };

// dynamic system network generators
template <class DynSys>
struct DynSysGen: public DynSys, public NetworkFromTimeSeries
{
  static int RKfunction(double t, const double x[], double f[], void *params)
  {
#if defined(GNUSL)
    if (params==NULL) return GSL_EBADFUNC;
    try
      {
        ((const DynSys*)params)->f(t,x,f);
      }
    catch (std::exception& e)
      {
        Tcl_AppendResult(interp(),e.what(),NULL);
        Tcl_AppendResult(interp(),"\n",NULL);
        return GSL_EBADFUNC;
      }
    return GSL_SUCCESS;
#else
    return 0;
#endif
  }
  // RK parameters
  double stepMin; // minimum step size
  double stepMax; // maximum step size
  double epsAbs;  // absolute error
  double epsRel;  // relative error
  DynSysGen(): stepMin(1e-5), stepMax(1e-3), epsAbs(1e-3), epsRel(1e-2) 
  {
    resize(DynSys::dim);
    n.resize(DynSys::dim,100);
  }

  void fillNet(int nsteps) {
#if defined(GNUSL)
    RKdata rkData(*this);
    vector<double> x(DynSys::dim);
    for (size_t i=0; i<x.size(); ++i)
      x[i]=uni.rand(); 
    double t=0;
    gsl_odeiv2_driver_set_nmax(rkData.driver, 1);
    for (int i=0; i<nsteps; ++i)
      {
        gsl_odeiv2_driver_apply(rkData.driver,&t,1,&x[0]);
        for (size_t j=0; j<size(); ++j)
          (*this)[j]<<=x[j];
      }
    constructNet();
#endif
  }
  double complexity() const {return ::complexity(net);}
  urand uni;
  void random_rewire() {::random_rewire(net,uni);}
};

// example dynamical systems defined here
struct Lorenz
{
  static const unsigned dim=3;
  double sigma, rho, beta;
  Lorenz(): sigma(10), rho(28), beta(8/3.0) {}
  void f(double t, const double x[3], double y[3]) const
  {
    y[0]=sigma*(x[1]-x[0]);
    y[1]=x[0]*(rho-x[2])-x[1];
    y[2]=x[0]*x[1]-beta*x[2];
  }
};

struct HenonHeiles
{
  static const unsigned dim=4;
  double lambda;
  HenonHeiles(): lambda(1) {}
  void f(double t, const double x[4], double y[4]) const
  {
    y[0]=x[1];
    y[1]=-x[0]-2*lambda*x[0]*x[2];
    y[2]=x[3];
    y[3]=-x[2]-lambda*(x[0]*x[0]-x[2]*x[2]);
  }
};

// small 1D cellular automata
struct Small1DCA
{
  unsigned rule; ///< rule in 1..255
  unsigned nCells; ///< no cells < 0..8*sizeof(state)
  unsigned long state;
  /// 3 bit neighbourhood of cell i
  unsigned nbhd(unsigned i)
  {
    if (i==0)
      return ((state&3)<<1) | ((state>>(nCells-1)) & 1);
    else if (i==nCells-1)
      return ((state>>(i-1))&3) | ((state&1)<<2);
    else
      return state>>(i-1)&7;
  }
  // ratio of 1 bits out of 8
  double lambda() {
    int sum=0;
    for (int i=0; i<8; ++i)
      sum+=(rule>>i)&1;
    return sum/8.0;
  }
  void step() {
    unsigned long newState=0;
    for (unsigned i=0; i<nCells; ++i)
      {
        unsigned n=nbhd(i);
        newState |= ((rule>>n) & 1) << i;
      }
    state=newState;
  }
};

struct Larger1DCA
{
  // number of states a cell can take, and neighbourhood size (always odd)
  unsigned nStates;
  int nbhdSz;
  vector<unsigned> rule,cells;
  urand uni;
  size_t nbhd(unsigned i)
  {
    size_t r=0;
    for (int j=-nbhdSz/2; j<=nbhdSz/2; ++j)
      {
        int ii=i+j;
        if (ii<0) ii+=cells.size();
        if (ii>int(cells.size())-1) ii-=cells.size();
        r*=nStates;
        r+=cells[ii];
      }
    return r;
  }
  // ratio of non-quiescent states in rule to total
  double lambda() {
    int sum=0;
    for (unsigned i=0; i<rule.size(); ++i)
      sum+=rule[i]>0;
    return double(sum)/rule.size();
  }
  // resize rule vector, given nStates and nbhdSz
  void resizeRule() {
    rule.resize(pow(nStates,nbhdSz));
  }
  // installs a random rule matching a given lambda
  void initRandomRule(double lambda) {
    resizeRule();
    for (size_t i=0; i<rule.size(); ++i)
      if (uni.rand()>lambda)
        rule[i]=0;
      else
        rule[i]=(nStates-1)*uni.rand()+1;
  }
  void initRandom() {
    for (size_t i=0; i<cells.size(); ++i)
      cells[i]=nStates*uni.rand();
  }
  void step() {
    vector<unsigned> newState(cells.size());
    for (unsigned i=0; i<cells.size(); ++i)
      newState[i] = rule[nbhd(i)];
    cells.swap(newState);
  }

};

template <class T>
struct NodeMap: map<T,unsigned>
{
  unsigned nextId;
  NodeMap(): nextId(0) {}
  unsigned operator[](const T& x) {
    typename map<T,unsigned>::iterator i=this->find(x);
    if (i!=this->end())
      return i->second;
    else
      return map<T,unsigned>::operator[](x)=nextId++;
  }
};

struct NetworkFromSmall1DCA: 
  public Small1DCA, public NetworkFromTimeSeries
{
  void fillNet(int nsteps, int transient) {
    resize(1); back().clear();
    state=pow(2,nCells)*uni.rand();
    NodeMap<unsigned long> nodeMap;
    
    // throw away the intial transient
    for (int i=0; i<transient; ++i) step();
    for (int i=0; i<nsteps; ++i) 
      {
        step();
        back()<<=nodeMap[state];
 //       if (nodeMap.nextId>10)
 //         cout << back() << " ";
      }
    //   cout<<endl;
    n.resize(1); n[0]=nodeMap.nextId;
    constructNet();
  }

  double complexity() const {return ::complexity(net);}
  urand uni;
  void random_rewire() {::random_rewire(net,uni);}

};

struct NetworkFromLarger1DCA: 
  public Larger1DCA, public NetworkFromTimeSeries
{
  void fillNet(int nsteps, int transient) {
    resize(1); back().clear();
    initRandom();
    // throw away the intial transient
    for (int i=0; i<transient; ++i) step();
    // map of state to node tag
    NodeMap<vector<unsigned> > nodeMap;
    unsigned tagid=1;
    for (int i=0; i<nsteps; ++i) 
      {
        step();
        back()<<=nodeMap[cells];
      }
    n.resize(1); n[0]=nodeMap.nextId;
    constructNet();
  }

  double complexity() const {return ::complexity(net);}
  urand uni;
  void random_rewire() {::random_rewire(net,uni);}

};


class netc_t
{
  archetype archc;
  Iterator<archetype> ic;
  bool fill_curr(); 
  CLASSDESC_ACCESS(netc_t);
public:
  netc_t(): weight_dist("one"), gProcess(g) {}
  unsigned currc;
  void reset_arch() {archc.clear();}
  bool first();
  bool next();
  bool last() {return ic==archc.end();}
  unsigned size() {return archc.size();}
  double ODcomplexity() {return ::ODcomplexity(g);}
  double MA() {return ::MA(g);}
  void exhaust_search(TCL_args);
  //  void gen_uniq_rand(TCL_args);  

  ConcreteGraph<DiGraph> g;
  Degrees degrees;
  void update_degrees() {degrees=::degrees(g);}
  urand uni;
  double complexity() {return ::complexity(g);}
  double lnomega() {BitRep eCanon; return canonical(eCanon,g);}
  double nauty_complexity() {return ::complexity(NautyRep(g));}
  double nauty_lnomega() {return NautyRep(g).lnomega();}
  double saucy_lnomega() {return ::saucy_lnomega(g);}

  /** race algorithms against each other in separate processes. Best
      run on a multicore machine with as many or more cores than there
      are algorithms (currently 4).

      @param time (in seconds) limiting the race
      @return the number of te winning algorithm, or 0 if the race times out
  */
  int automorphism_race(TCL_args);
  /**
     races variants of the bliss algorithm with different values of
     the splitting heuristic 
     @param time (in seconds) limiting the  race 
     @return the winning splitting heuristic, or -1 if the race times out
  */
  int bliss_race(TCL_args);
  /// returns a string representation of the algorithm given by the
  /// integer argument
  string automorphism_algorithm_name(TCL_args);

  TCL_obj_ref<random_gen> weight_dist;
  array<float> weights() {
    array<float> r;
    for (ConcreteGraph<DiGraph>::const_iterator e=g.begin(); e!=g.end(); ++e)
      r<<=e->weight;
    return r;
  }
  void genRandom(TCL_args args) {
    ErdosRenyi_generate(g,args[0],args[1],uni,*weight_dist);}
  void prefAttach(TCL_args args) {
    TCL_obj_ref<random_gen> indegree_dist;
    indegree_dist.set(args[1]);
    PreferentialAttach_generate(g,args[0],uni,*indegree_dist,*weight_dist);
  }
  void random_rewire() {::random_rewire(g,uni);}
  /// generate a fully connected directed graph, with each node having
  /// same indegree and outdegree. Only works if graph degree is odd.
  void genFullDirected(TCL_args args);
  bool testAgainstNauty();

  GaussianMarkovModel gProcess;

  DynSysGen<Lorenz> lorenzGen;
  DynSysGen<HenonHeiles> henonHeilesGen;
  NetworkFromSmall1DCA small1DCA;
  NetworkFromLarger1DCA large1DCA;
};

