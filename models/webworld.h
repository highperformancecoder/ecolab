/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <vector>
using std::vector;
#include <pack_stl.h>
#include <netcomplexity.h>

class array2
{
  CLASSDESC_ACCESS(array2);
  int rows, cols;
  array<double> val;
public:
  array2(int r=1, int c=0): rows(r), cols(c), val(r*c) {}
  array2& operator=(double& x) {val=x; return *this;}
  double& operator() (int i, int j) {return val[i+j*rows];}

  /* conversions to/from sparse_mats */
  array2(sparse_mat& x): rows(x.rowsz), cols(x.colsz), val(x.rowsz*x.colsz)
  {
    val=0;
    for (size_t i=0; i<x.val.size(); i++) (*this)(x.row[i],x.col[i])=x.val[i];
  }
  operator sparse_mat()
  {
    sparse_mat r;
    for (int i=0; i<rows; i++)
      for (int j=0; j<cols; j++)
	if ((*this)(i,j)!=0) 
	  {
	    r.val<<=(*this)(i,j);
	    r.row<<=i;
	    r.col<<=j;
	  }
    return r;
  }

};

class webworld_t: public TCL_obj_t
{
  CLASSDESC_ACCESS(webworld_t);
  /* utility members for generate */
  array<double> sumaSfN(array2& a, sparse_mat& f);
  void compute_g(sparse_mat& g);

  /* utility for mutate */
  bool genotype_exists(const array<int>& g);
public:
  int K; /* poolsize of attributes */
  int L;  /* number of attributes of a species */
  array2 M; /* matrix of payoffs between one feature and another */
  vector<array<int> > genotype;
  sparse_mat S, f, g; /* interaction matrix */
  void initM(TCL_args);
  void init_interaction(TCL_args);

  gaussrand grand; /* random generator used for generating M */
  urand unirand;  /* used for mutate algorithm */


  double t;     /* time */
  array<double> N;   /* population counts */
  array<int> species; /* tags */
  array<double> create;   /* creation time */
  double lambda;  /* ecological efficiency */
  double  b, c;
  double R; /* Resource productivity */
  double fmin; /* minimum effort */

  /* set some default parameters */
  webworld_t(): K(500), L(10), t(0), lambda(0.1), b(5E-3), c(0.5), R(10E5), 
		fmin(1E-6) {}

  void generate(TCL_args);
  double dissipation();

  int condense();
  void mutate();

  array<double> lifetimes();
  double conn();
  sparse_mat_graph foodweb;

  double complexity()
  {
    return ::complexity(foodweb);
  }
};
