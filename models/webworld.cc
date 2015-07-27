/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "arrays.h"
using namespace ecolab;
using array_ns::array;
#include <sparse_mat.h>
#include "ecolab.h"
#include "webworld.h"
#include "webworld.cd"
#include "ecolab_epilogue.h"

webworld_t webworld;
make_model(webworld);

void webworld_t::initM(TCL_args args)
{
  K=args; 
  M=array2(K,K);

  for (int i=0; i<K; i++)
    {
      M(i,i)=0;
      for (int j=0; j<i; j++)
	{
	  M(i,j)=grand.rand();
	  M(j,i)=-M(i,j);
	}
    }
}


void webworld_t::init_interaction(TCL_args args)
{
  int i,j,k,l,m;
  L=args;
  genotype.resize(N.size());
  species=array_ns::pcoord(N.size());
  create=array<double>(N.size())=0;
  for (size_t i=0; i<N.size(); i++)
    {
      genotype[i]=array<int>(L);
      fill_unique_rand(genotype[i],K);
    }
  S=sparse_mat();
  for (size_t i=0, m=0; i<N.size(); i++)
    for (size_t j=0; j<i; j++)
      {
	double s=0;
	for (size_t k=0; k<genotype[i].size(); k++)
	  for (size_t l=0; l<genotype[j].size(); l++)
	    s+=M( genotype[i][k], genotype[j][l] );
	if (s>0 && j!=0) {S.val<<=s/L; S.col<<=i; S.row<<=j;}
	if (s<0) {S.val<<=-s/L; S.col<<=j; S.row<<=i;}
      }
  S.rowsz=S.colsz=N.size();
  f=S;  /* uniform initial value of feeding strategies */
  f.val=1;
  array<double> one(N.size()); one=1;
  array<double> colsum(1.0/(f*one));
  f.val=colsum[f.row];
}
	    
array<double> webworld_t::sumaSfN(array2& a, sparse_mat& Sf)
{
  vector<sparse_mat> SF(f.rowsz); 
  int i,k;
  for (size_t i=0; i<f.val.size(); i++)
    {
      SF[f.col[i]].val<<=Sf.val[i];
      SF[f.col[i]].row<<=Sf.row[i];
      //SF[f.col[i].col<<=Sf.col[i];  //don't actually need column values
    }

  array<double> r(f.val.size()); 
  for (size_t i=0; i<r.size(); i++)
    {
      r[i]=0;
      sparse_mat& col=SF[f.col[i]];
      for (size_t k=0; k<col.val.size(); k++)
	r[i]+=a(col.row[k],f.row[i])*col.val[k]*N[col.row[k]];
    }
  return r;
}


void webworld_t::compute_g(sparse_mat& g)
{

  array2 a(N.size(),N.size()); /* similarity matrix */
  for (size_t i=0; i<N.size(); i++)
    {
      a(i,i)=1;
      for (size_t j=0; j<i; j++)
	{
	  int common_features=0;
	  for (size_t k=0; k<genotype[i].size(); k++)
	    for (size_t l=0; l<genotype[j].size(); l++)
	      common_features += genotype[i][k]==genotype[j][l];
	  assert(common_features<=L);
	  a(j,i)=a(i,j) = c+(1-c)*common_features/double(L);
	}
    }

  array<double> one(N.size()); one=1;
  f=S;  /* reset f to uniform feeding strategies */
  f.val=1;
  array<double> colsum(1.0/(f*one));
  f.val=colsum[f.row];
  array<double> fp;
  sparse_mat Sf=S; Sf.val*=f.val; 
  array<double> denom(b*N[S.col] +  sumaSfN(a,Sf));
  g.val = merge( denom>0, Sf.val*N[S.col] / denom, 0.0);
   
  int ctr=0;
  do 
    {
      fp=f.val;
      array<double> colsum(1.0/(g*one));
      f.val = g.val *colsum[g.row];
      f.val = merge( f.val>=fmin, f.val, fmin);
      Sf.val=S.val*f.val;
      denom = b*N[S.col] +  sumaSfN(a,Sf);
      g.val = merge( denom>0, Sf.val*N[S.col] / denom, 0.0);
    } while (sum(abs(f.val-fp) > 0.1*f.val) > 0);
}

void webworld_t::generate(TCL_args args)
{
  double step=args;
  N[0]=R*step/lambda;   /* fixed resource production */

  array<double> one(N.size()); one=1;
  /*sparse_mat*/ g=S; compute_g(g);
  foodweb = g;

  N += step*(-N + lambda*N*(g*one) - tr(g) * N);
  N=merge(N<1,0.0,N);     /* pop < 1 => extinction */
  t+=step;
  N[0]=1;
}

double webworld_t::dissipation()
{
  double r=0;
  int i;
  for (size_t i=1; i<N.size(); i++) r+=N[i];
  for (size_t i=0; i<g.val.size(); i++) r+=(1-lambda)*N[g.row[i]]*g.val[i];
  return r;
}

template <class T>
vector<T> pack(const vector<T> x, array<int> mask, int ntrue=-1)
{
  vector<T> r(ntrue==-1? sum(mask): ntrue);
  for (size_t i=0, j=0; i<mask.size(); i++) 
    if (mask[i]) r[j++]=x[i];
  return r;
}

int webworld_t::condense()
{
  array<int> mask(N>0);
  int ntrue=sum(mask);
  N=pack(N,mask,ntrue);
  species=pack(species,mask,ntrue);
  genotype=pack(genotype,mask,ntrue);
  array<int> mask_off(mask[S.row]==1 && mask[S.col]==1);
  array<int> map(enumerate(mask));
  int noff=sum(mask_off);
  f.rowsz=f.colsz=S.rowsz=S.colsz=N.size();
  S.val=pack(S.val,mask_off,noff);
  f.val=pack(f.val,mask_off,noff);
  f.row=S.row=map[pack(S.row,mask_off,noff)];
  f.col=S.col=map[pack(S.col,mask_off,noff)];
  g.val=pack(g.val,mask_off,noff);
  g.row=map[pack(g.row,mask_off,noff)];
  g.col=map[pack(g.col,mask_off,noff)];
  foodweb = g;
  return mask.size()-ntrue;
}


/* first version is as per original Web World - one species is selected at random, one trait is randomly replaced, and process iterated until a new species is created. */

bool webworld_t::genotype_exists(const array<int>& g)
{
  for (size_t i=0; i<genotype.size(); i++)
    if (unsigned(sum(genotype[i]==g))==g.size()) return true;
  return false;
}

void webworld_t::mutate()
{
  int parent_species=(N.size()-1)*unirand.rand()+1;
  array<int> new_gene=genotype[parent_species];
  int site;
  do
    {
      new_gene=genotype[parent_species];
      site=new_gene.size() * unirand.rand();
      new_gene[site]=(int)(new_gene[site]+K*unirand.rand())%K;
    } while (genotype_exists(new_gene) || array_ns::sum(new_gene==new_gene[site])>1 );

  /* add new species */
  N<<=1;
  species<<=max(species)+1;
  create<<=0;
  genotype.push_back(new_gene);

  /* update interaction matrix */
  int i=N.size()-1,j,k,l;
  for (j=0; j<i; j++)
    {
      double s=0;
      for (size_t k=0; k<genotype[i].size(); k++)
	for (size_t l=0; l<genotype[j].size(); l++)
	  s+=M( genotype[i][k], genotype[j][l] );
      if (s>0 && j!=0) {S.val<<=s/L; S.col<<=i; S.row<<=j;}
      if (s<0) {S.val<<=-s/L; S.col<<=j; S.row<<=i;}
    }
  S.rowsz=S.colsz=N.size();

  f=S;  /* reset f to uniform feeding strategies */
  f.val=1;
  array<double> one(N.size()); one=1;
  array<double> colsum(1.0/(f*one));
  f.val=colsum[f.row];

}


array<double> webworld_t::lifetimes()
{ 
  array<double> lifetimes; 

  for (size_t i=0; i<N.size(); i++) 
    {
      if (create[i]==0 && N[i]>10) 
	create[i] = t;
      else if (create[i]>0 && N[i]==0) 
	/* extinction */
	{
	  lifetimes <<= t - create[i];
	  create[i]=0;
	}
    }
  return lifetimes;
}


double webworld_t::conn()
{
  double r=0;
  for (size_t i=0; i<f.val.size(); i++) r+=f.val[i]>fmin;
  return r/(N.size()*N.size());
}
