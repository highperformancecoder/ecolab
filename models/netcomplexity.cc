/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
using namespace classdesc;

#include "graph.h"
#include "netcomplexity.h"
#include "netcomplexity_model.h"
#include "netcomplexity_model.cd"
#include "ecolab_epilogue.h"

#ifdef GNUSL
#include <gsl/gsl_matrix.h>
// a bit of idiocy to get around GSL's use of a typedef
struct Gsl_matrix: public gsl_matrix {};
void pack(pack_t&, const string&, Gsl_matrix&) {}
void unpack(pack_t&, const string&, Gsl_matrix&) {}
#else
struct Gsl_matrix {};
void pack(pack_t&, const string&, Gsl_matrix&) {}
void unpack(pack_t&, const string&, Gsl_matrix&) {}
#endif

#ifdef IGRAPH
#include "igraph.h"
#endif
#include "ecolab_epilogue.h"
#include <signal.h>
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
#include <sys/wait.h>
#endif

#include <fstream>
using namespace std;

using std::next_permutation;
using namespace classdesc;

const unsigned Lorenz::dim;
const unsigned HenonHeiles::dim;

make_model(one);

netc_t netc;
make_model(netc);

bool netc_t::fill_curr() 
{
  if (ic==archc.end()) 
    return false;
  currc=ic->second; 
  g=ic->first;
  return true;
}

bool netc_t::first()
{
  ic=archc.begin();
  return fill_curr();
}

bool netc_t::next()
{
  ic++;
  return fill_curr();
}

void merge(archetype& x, const archetype& y)
{
  for (archetype::const_iterator i=y.begin(); i!=y.end(); i++)
    x[i->first]+=i->second;
}

void netc_t::genFullDirected(TCL_args args)
{
  unsigned nodes=args;
  for (unsigned i=0; i<nodes; ++i)
    for (unsigned j=0; j<nodes; ++j)
      if (j!=i) 
        {
          if ((i+j)%2==0)
            g.push_back(Edge(i,j));
          else
            g.push_back(Edge(j,i));
        }
}
 
void netc_t::exhaust_search(TCL_args args)
{
  unsigned n=args, l=args;
  BitRep x(n,l);
  
  for (bool run=true; run; run=x.next_perm() )
    archc[canonicalise(x)]++;
}


bool netc_t::testAgainstNauty()
{
  NautyRep nCanon;
  BitRep eCanon, enCanon;
  double nlnomega, elnomega;
  clock_t t0=clock();
  ecolab_nauty(g,nCanon,nlnomega,true);
  clock_t t1=clock();
  elnomega=canonical(eCanon,g);
  bool OK=fabs(elnomega-nlnomega)<log(1.5);
  clock_t t2=clock();
  elnomega=canonical(enCanon,nCanon);
  OK&=fabs(elnomega-nlnomega)<log(1.5);
  OK&=eCanon==enCanon;

  cout << elnomega << " "<<nlnomega;

  // Saucy implementation produces slightly the wrong answer!
//#ifdef SAUCY
//  double slnomega=::saucy_lnomega(g);
//  OK&=fabs(nlnomega-slnomega)<log(1.5);
//  cout << " "<<slnomega;
//#endif

// igraph routines fail this test!
//#ifdef IGRAPH
//  double iglnomega0=::igraph_lnomega(g,0), iglnomega2=::igraph_lnomega(g,2);
//  OK&=fabs(nlnomega-iglnomega0)<log(1.5);
//  OK&=fabs(nlnomega-iglnomega2)<log(1.5);
//  cout << " "<<iglnomega0<<" "<<iglnomega2;
//#endif
//  cout <<endl;

  return OK;
}

void finish_race(int pipes[2], int w)
{
  const int read_pipe=0, write_pipe=1;
  if (write(pipes[write_pipe], &w, sizeof(w))<0)
    puts(strerror(errno));
  close(pipes[0]); close(pipes[1]);
  exit(0);
}

#ifdef IGRAPH
igraph_bool_t compat
(const igraph_t *,const igraph_t *,const igraph_integer_t,
 const igraph_integer_t,void *arg) {return true;}
#endif

int netc_t::automorphism_race(TCL_args args)
{
#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
  int timeout=args;
  // set up return pipe
  const int read_pipe=0, write_pipe=1;
  int pipes[2];
  if (pipe(pipes)!=0)
    throw error(strerror(errno));

  int method=0;

  vector<pid_t> children;
  // first process just sleeps (the timeout)
  children.push_back(fork());
  if (children.back()==0)
    {
      sleep(timeout);
      finish_race(pipes, method);
    }
  else if (children.back() == -1)
    throw error(strerror(errno));

  method++;
//  children.push_back(fork());
//  if (children.back()==0)
//    {
//      BitRep canon;
//      canonical(canon,g);
//      finish_race(pipes, method);
//    }
//  else if (children.back() == -1)
//      throw error(strerror(errno));

  method++;
  children.push_back(fork());
  if (children.back()==0)
    {
      NautyRep canonical;
      double lnomega;
      ecolab_nauty(NautyRep(g), canonical, lnomega, false, 0);
      finish_race(pipes, method);
    }
  else if (children.back() == -1)
      throw error(strerror(errno));

  method++;
  children.push_back(fork());
  if (children.back()==0)
    {
      NautyRep canonical;
      double lnomega;
      ecolab_nauty(NautyRep(g), canonical, lnomega, false, 14);
      finish_race(pipes, method);
    }
  else if (children.back() == -1)
      throw error(strerror(errno));


#ifdef SAUCY 
  method++;
  children.push_back(fork());
  if (children.back()==0)
    {
      ecolab::saucy_lnomega(g);
      finish_race(pipes, method);
    }
  else if (children.back() == -1)
      throw error(strerror(errno));
#endif

#ifdef IGRAPH
  method++;
  children.push_back(fork());
  if (children.back()==0)
    {
      IGraph ig(g);
      igraph_integer_t count;
#ifdef IGRAPH_VERSION
      igraph_count_isomorphisms_vf2(&ig,&ig,NULL,NULL,NULL,NULL,&count,compat,compat,NULL);
#else
      igraph_count_isomorphisms_vf2(&ig,&ig,&count);
#endif
      finish_race(pipes, method);
    }
    else if (children.back() == -1)
      throw error(strerror(errno));

  for (int w=0; w<3; ++w)
    {
      method++;
      children.push_back(fork());
      if (children.back()==0)
        {
          IGraph ig(g);
          igraph_bliss_sh_t sh(static_cast<igraph_bliss_sh_t>(w));
          igraph_bliss_info_t ret;
          igraph_automorphisms(&ig,sh,&ret);
          finish_race(pipes, method);
        }
      else if (children.back() == -1)
        throw error(strerror(errno));
    }

#endif

    int winner;
    if (read(pipes[read_pipe],&winner,sizeof(winner))<0)
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

    return winner;
#else
    return 0;
#endif
}

int netc_t::bliss_race(TCL_args args)
{
#if defined(IGRAPH) && !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
  int timeout=args;
  // set up return pipe
  const int read_pipe=0, write_pipe=1;
  int pipes[2];
  if (pipe(pipes)!=0)
    throw error(strerror(errno));

  vector<pid_t> children;
  // first process just sleeps (the timeout)
  children.push_back(fork());
  if (children.back()==0)
    {
      sleep(timeout);
      int w=-1;
      if (write(pipes[write_pipe], &w, sizeof(w))<0)
        puts(strerror(errno));
      close(pipes[0]); close(pipes[1]);
      exit(0);
    }
  else if (children.back() == -1)
    throw error(strerror(errno));

  for (int w=0; w<6; ++w)
    {
      children.push_back(fork());
      if (children.back()==0)
        {
          IGraph ig(g);
          igraph_bliss_sh_t sh(static_cast<igraph_bliss_sh_t>(w));
          igraph_bliss_info_t ret;
          igraph_automorphisms(&ig,sh,&ret);
          if (write(pipes[write_pipe], &w, sizeof(w))<0)
            puts(strerror(errno));
          close(pipes[0]); close(pipes[1]);
          exit(0);
        }
      else if (children.back() == -1)
        throw error(strerror(errno));
    }

    int winner;
    if (read(pipes[read_pipe],&winner,sizeof(winner))<0)
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

    return winner;
#else
    return 0;
#endif
}

string netc_t::automorphism_algorithm_name(TCL_args args)
{
  switch(int(args))
    {
    case 0: return "Timed out";
    case 1: return "SuperNOVA";
    case 2: return "Nauty";
    case 3: return "Saucy";
    case 4: return "VF2";
    default: return "Unknown";
    }
}

#ifndef GNUSL
GaussianMarkovModel::~GaussianMarkovModel() {}
void GaussianMarkovModel::run(TCL_args repetitions) {}
double GaussianMarkovModel::causalDensity() {
  throw error("not implemented");return 0;}
double GaussianMarkovModel::bipartitionCausalDensity() {
  throw error("not implemented");return 0;}
void GaussianMarkovModel::outputRunData(TCL_args filename)
{throw error("not implemented");}
#else

#include <gsl/gsl_linalg.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_blas.h>

GaussianMarkovModel::~GaussianMarkovModel() 
{
  if (runData)
    gsl_matrix(free);
}

// adaptor to vectors easier to manipulate
struct Vector: public gsl_vector
{
  Vector() {gsl_vector::size=0; data=NULL;}
  Vector(const gsl_vector& v): gsl_vector(v) {}
  virtual ~Vector() {}
  double& operator[](size_t i) {return *gsl_vector_ptr(this,i);}
  double operator[](size_t i) const {return gsl_vector_get(this,i);}
  Vector subvector(size_t offs, size_t len) {
  return Vector(gsl_vector_subvector(this,offs,len).vector);
  }
  size_t size() const {return gsl_vector::size;}
  /// vector copy operation
  const Vector& operator=(const Vector& x) {
    gsl_vector_memcpy(this, &x);
    return *this;
  }
  const Vector& operator=(double x) {
    gsl_vector_set_all(this,x);
    return *this;
  }
};


// concrete vector (manages object lifecycle)
struct CVector: public Vector
{
  CVector(size_t s=0) {
    gsl_vector::size=s;
    stride=1;
    data = new double[s];
    block=NULL;
    owner=0;
  }
  ~CVector() {delete [] data;}
  void resize(size_t s) {
    if (s!=size())
      {delete [] data; data=new double[s]; gsl_vector::size=s;}
  }
private:
  // copy operations private, because of the owned pointer
  CVector(const CVector&);
  void operator=(const CVector&);
};

// adaptor to matrices easier to manipulate
struct Matrix: public gsl_matrix
{
  Matrix() {size1=size2=0; data=0;}
  Matrix(const gsl_matrix& m): gsl_matrix(m) {}
  double& operator()(size_t i, size_t j)
  {return *gsl_matrix_ptr(this,i,j);}
  double operator()(size_t i, size_t j) const 
  {return gsl_matrix_get(this,i,j);}
  const Matrix& operator=(double x)
  {gsl_matrix_set_all(this,x); return *this;}
  const Matrix& operator=(const Matrix& x)
  {gsl_matrix_memcpy(this, &x); return *this;}
   
  size_t rows() const {return size1;}
  size_t cols() const {return size2;}
  Vector row(size_t i) {return Vector(gsl_matrix_row(this,i).vector);}
  Vector col(size_t i) {return Vector(gsl_matrix_column(this,i).vector);}
  const Vector row(size_t i) const {return Vector(gsl_matrix_const_row(this,i).vector);}
  const Vector col(size_t i) const {return Vector(gsl_matrix_const_column(this,i).vector);}
  Vector diag() {return Vector(gsl_matrix_diagonal(this).vector);}
  const Vector diag() const {return Vector(gsl_matrix_const_diagonal(this).vector);}
};

// concrete matrix (manages object lifecycle)
struct CMatrix: public Matrix
{
  CMatrix(size_t m=0, size_t n=0) {
    size1=m; tda=size2=n;
    data=new double[m*n];
    block=NULL; owner=0;
  }
  ~CMatrix() {delete [] data;}
  void resize(size_t r, size_t c) 
  {
    if (r!=rows() || c!=cols())
      {delete [] data; size1=r; tda=size2=c; data=new double[r*c];}
  }
  const CMatrix& operator=(const Matrix& x) {(Matrix&)(*this)=x; return *this;}
private:
  // copy operations private, because of the owned pointer
  CMatrix(const CMatrix&);
  void operator=(const CMatrix&);
};

double logDet(const Matrix& m)
{
  CMatrix copy(m.rows(), m.cols());
  copy=m;
  gsl_permutation *p=gsl_permutation_alloc(m.rows());
  int s;
  gsl_linalg_LU_decomp(&copy, p, &s);
  gsl_permutation_free(p);
  return gsl_linalg_LU_lndet(&copy);
}

std::ostream& operator<<(ostream& o, const Vector& v)
{
  for (size_t i=0; i<v.size(); ++i) 
    o<<((i==0)?"":" ")<<v[i]; 
  return o;
}
std::ostream& operator<<(ostream& o, const Matrix& m)
{
  for (size_t i=0; i<m.rows(); ++i) 
    {
      for (size_t j=0; j<m.cols(); ++j)
        o<<m(i,j)<<" ";
      o<<std::endl;
    }
  return o;
}
std::ostream& operator<<(ostream& o, const CVector& v)
{return o<<static_cast<const Vector&>(v);}
std::ostream& operator<<(ostream& o, const CMatrix& m)
{return o<<static_cast<const Matrix&>(m);}

void GaussianMarkovModel::run(TCL_args repetitions) 
{
  if (runData) gsl_matrix_free(runData);
  size_t samples=int(repetitions)+lags;
  runData=static_cast<Gsl_matrix*>(gsl_matrix_alloc(g.nodes(), samples));

  Matrix m(*runData);
  for (size_t i=0; i<samples; ++i)
    {
      // initialise sample to a set of random numbers
      for (size_t j=0; j<g.nodes(); ++j)
        {
          m(j,i)=gran.rand();
          //            cout << "xx" << g.links() << " " << m(j, i) <<" "<<endl;
        }
      if (i>0)
        for (G::const_iterator e=g.begin(); e!=g.end(); ++e)
          {
            //            cout << "xx" << g.links() << " " << m(e->target(), i) <<" "<< m(e->source(), i-1) << " "<<e->weight<<endl;
            m(e->target(), i) += m(e->source(), i-1) * e->weight / g.links();
          }
    }
  //  cout << "m:"<<gsl_matrix_max(&m) << " " << gsl_matrix_min(&m) << endl;
  //  cout << "runData"<<gsl_matrix_max(runData) << " " << gsl_matrix_min(runData) << endl;


}

void GaussianMarkovModel::outputRunData(TCL_args filename)
{
  if (!runData)
    throw error("You need to run a simulation before output run data");
  Matrix m(*runData);
  std::ofstream f((char*)filename);
  for (size_t r=0; r<m.rows(); ++r)
    {
      for (size_t c=0; c<m.cols(); ++c)
        f << m(r,c) << " ";
      f << endl;
    }
} 

double covarianceMatrixFromData
(CMatrix& covar, const Vector& depVar, const Matrix& predVar)
{
  CVector A(predVar.cols());
  covar.resize(predVar.cols(), predVar.cols());
  double chisq=0;
  gsl_multifit_linear_workspace *w=
    gsl_multifit_linear_alloc(predVar.rows(), predVar.cols());
  gsl_multifit_linear(&predVar, &depVar, &A, &covar, &chisq, w);
  gsl_multifit_linear_free(w);

  //  cout << "chisq="<<chisq<<endl;
  return chisq;
}
                              
void predictorMatrixFromData(CMatrix& pred, 
                             const Matrix& data, int lags, 
                             int rowToExclude=-1)
{
  size_t nObs=data.cols()-lags-1;
  // number of predictor variables
  size_t nPred=lags*(rowToExclude<0? data.rows(): data.rows()-1);
  pred.resize(nObs, nPred);

  for (size_t row=0, p=0; row<data.rows(); ++row)
    if (int(row)!=rowToExclude)
      {
        Vector rdata(data.row(row));
        for (int offs=0; offs<lags; ++offs)
          pred.col(p++) = rdata.subvector(offs, nObs);
      }
}

double GaussianMarkovModel::causalDensity() 
{
  if (!runData) return 0;

  double sumF=0;

  Matrix data(*runData);

  // Covariances of nodes
  CMatrix covar, pred;
  predictorMatrixFromData(pred, data, lags);
  CVector logVar(g.nodes());

  for (size_t n=0; n<g.nodes(); ++n)
    {
      Vector obs(data.row(n).subvector(lags, pred.rows()));
      logVar[n]=covarianceMatrixFromData(covar, obs, pred);
      //      logDetCovar[n]=logDet(covar);
      if (n==0 && !isfinite(logVar[n]))
        cout << g << endl;
    }
  
  for (G::const_iterator e=g.begin(); e!=g.end(); ++e)
    {
      predictorMatrixFromData(pred, data, lags, e->source());
      Vector obs(data.row(e->target()).subvector(lags, pred.rows()));
      sumF+=log(covarianceMatrixFromData(covar, obs, pred)/logVar[e->target()]);
    }
  return sumF; ///g.links();

}


double GaussianMarkovModel::bipartitionCausalDensity() {return 0;}

#endif
