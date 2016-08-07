/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* The good old ecolab model that also has a Mark Bedau neutral shadow
   model "joined-at-the-hip". See paper "Ecolab perspective on Bedau
   evolutionary statistics" for more details. */

#include "ecolab.h"
#include "shadow_model.h"
#include "shadow_model.cd"
#include "ecolab_epilogue.h"

using namespace std;

namespace
{
  shadow_model ecolab;

  make_model(ecolab);
}

void shadow_model::random_interaction(TCL_args args)
{interaction.init_rand(args[0], args[1]);}

/* Rounding function, randomly round up or down, in the range 0..INT_MAX */
inline int ROUND(double x) 
{
  double dum;
  if (x<0) x=0;
  if (x>INT_MAX-1) x=INT_MAX-1;
  return std::fabs(std::modf(x,&dum)) > array_urand.rand() ?
    (int)x+1 : (int)x;
}

inline int ROUND(float x) {return ROUND(double(x));}

template <class E>
inline ecolab::array<int> ROUND(const E& x)
{
  ecolab::array<int> r(x.size());
  for (size_t i=0; i<x.size(); i++)
    r[i]=ROUND(x[i]);
  return r;
}

void shadow_model::generate()
{ 
  ecolab::array<double> tmp = repro_rate * density + (interaction * density) * density;
  density = ROUND(density + tmp);

  if (tstep==0) make_consistent();
  tstep++;

  /* generate a random permutation */
  ecolab::array<int> t(density.size());
  fill_unique_rand(t,t.size());
  sdensity += tmp[t];
  activity += ((density-sdensity)>0) * (density-sdensity);
}

void shadow_model::condense()   /* remove extinct species */
{
  ecolab::array<int> mask, map, mask_off, extinctions;
  unsigned mask_true;
  mask = density != 0;
  mask_true=sum(mask);
  if (density.size()==mask_true) return; /* no change ! */

  map = enumerate( mask );
  mask_off = density[ interaction.row ] !=0 && density[ interaction.col ] !=0;

  density = pack( density, mask, mask_true); 
  sdensity = pack( sdensity, mask, mask_true); 
  activity = pack( activity, mask, mask_true); 
  repro_rate = pack(repro_rate, mask, mask_true); 
  mutation = pack(mutation, mask, mask_true); 
  create = pack(create, mask, mask_true); 
  species = pack(species, mask, mask_true); 
  interaction.diag = pack(interaction.diag, mask, mask_true);
  mig_ns = pack(mig_ns, mask, mask_true);
  mig_ew = pack(mig_ew, mask, mask_true);


  mask_true=sum(mask_off);

  interaction.val = pack(interaction.val, mask_off, mask_true); 
  interaction.row = map[pack(interaction.row, mask_off, mask_true)];
  interaction.col = map[pack(interaction.col, mask_off, mask_true)];

  //  assert(sum(interaction.diag>=0)==0);
  //  assert(sum(mutation<0)==0);
}

/* 
Vary components according to a Gaussian distribution, with the given
  std deviation 
*/

/* do the offdiagonal mutations */
void do_row_or_col(ecolab::array<double>& tmp, double range, double minval, double gdist)
{
  double r;
  int j, pos;
  unsigned ntrue;
  tclvar gen_bias("generalization_bias"); /* gen \in [-1,1] */
  double gen= exists(gen_bias)? (double)gen_bias:0.0;

  /* create or delete some connections */
  r = (2.0*rand())/RAND_MAX - 1+gen;
  r = (r>0)? r/(1+gen): r/(1-gen); 
  if (r!=0) ntrue=(int)(1/fabs(r))-1;
  else ntrue = tmp.size();

  if (ntrue>tmp.size()) ntrue=tmp.size();

  if (r>0)
    for (unsigned j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size()-1) * ((float) rand()/RAND_MAX) +.5);
	if (tmp[pos]==0.0)
	  tmp[pos] = range *((float) rand()/RAND_MAX) + minval;
      }
  else
    for (unsigned j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size()-1) * ((float) rand()/RAND_MAX) +.5);
	tmp[pos]=0;
      }

  /* mutate values */
  ecolab::array<double> diff(tmp.size());
  diff=merge(tmp!=0.0,range*gdist,0.0);
  gspread(tmp,diff);
}  

void shadow_model::mutate()
{
  ecolab::array<int> new_sp;
  ecolab::array<double> new_repro_rate, new_mutation, new_idiag, new_mig_ns, new_mig_ew;
  int i, cell, j, ntrue;
  static int sp_cntr=1;    /* used to label newly created species */

  /* ensure sp_cntr operates in a different domain on each
     processor. This is done by initialising it with
     max(species)+myid+1, then incrementing it by nprocs each time a
     new species is generated */

  if (sp_cntr==1)
    sp_cntr=max(species)+myid+1;

  /* calculate the number of mutants each species produces */
  new_sp = (double) sp_sep * repro_rate * mutation * density * 
    (tstep-last_mut_tstep);
  last_mut_tstep=tstep;

  /* adjust density by mutant values i.e. consider that some organisms
     born in previous step are now discovered to be mutants */
  density-=new_sp;
  sdensity-=new_sp;

  /* generate index list of old species that mutate to the new */
  ecolab::array<int> t=new_sp;
  new_sp = gen_index(new_sp); 

  if (new_sp.size()==0) return;

  /* adjust row and col to "within cell" coordinates */

  /* collect the old phenotypes */
  new_repro_rate = repro_rate[new_sp];
  new_idiag = interaction.diag[new_sp];
  new_mutation = mutation[new_sp];
  new_mig_ns = mig_ns[new_sp];
  new_mig_ew = mig_ew[new_sp];

  /* calculate the genetic distances for the mutants from the parents*/
  ecolab::array<double> gdist(new_sp.size());
  fillprand(gdist);
  gdist *= new_mutation;

  /* 
     change phenotypes randomly, according a normal distribution 
     with std dev gdist 
     */
  double range = repro_max-repro_min;

  gspread( new_repro_rate, range*gdist );
  ecolab::array<double> a=new_idiag;
  lgspread( new_idiag, gdist );
  lgspread( new_mutation, gdist );
  lgspread( new_mig_ns, gdist );
  lgspread( new_mig_ew, gdist );

  /* limit idiag to avoid it vanishing (causes system instability) - a
     reasonable is to chose it so that the equilibrium value of
     density is always less than half of INT_MAX */
  ecolab::array<double> max_idiag(-abs(new_repro_rate)/(0.1*INT_MAX));
  new_idiag = array_ns::merge( new_idiag < max_idiag, new_idiag, max_idiag);

  /* limit mutation rate to mutation(random,maxval) */
  new_mutation = merge( new_mutation < mut_max, new_mutation, mut_max);

  repro_rate <<=  new_repro_rate;
  interaction.diag <<=  new_idiag;
  mutation <<= new_mutation;
  mig_ns <<= new_mig_ns;
  mig_ew <<= new_mig_ew;

  /* vary offdiagonal elements */
  range =  odiag_max-odiag_min;

  /* collect interaction data */
  for (size_t i=0; i<new_sp.size(); i++)
    {
      ecolab::array<double> tmp1(species.size()+i), tmp2(species.size()+i);
      double s;
      tmp1 = 0; tmp2=0;
      ecolab::array<int> pcd = pcoord(tmp1.size());
	      
      /* project out connections for row[new_sp[i]] */
      ecolab::array<int> mask(interaction.row==unsigned(new_sp[i]));
      int ntrue = sum(mask);
      tmp1[ pack(interaction.col,mask,ntrue) ] = 
	pack(interaction.val,mask,ntrue);
      tmp1[ new_sp[i] ] = interaction.diag[new_sp[i]];

      do_row_or_col(tmp1,range,odiag_min,gdist[i]);
      
      /* project out connections for col[new_sp[i]] */
      mask = interaction.col==unsigned(new_sp[i]);
      ntrue = sum(mask);
      tmp2[ pack( interaction.row, mask, ntrue) ] = 
	pack( interaction.val, mask, ntrue);
      tmp2[ new_sp[i] ] = interaction.diag[new_sp[i]];

      do_row_or_col(tmp2,range,odiag_min,gdist[i]);

      /* adjust offdiag vals so that n.\beta n<0 for all positive n */
      /* construct list of offdiag vals that sum positively */
      mask = tmp1+tmp2>0;
      s=sum(tmp1+tmp2,mask)+new_idiag[i];
      if (s>0)
	{
	  int nadj;
	  ecolab::array<int> m1 = mask && tmp1!=0.0;
	  ecolab::array<int> m2 = mask && tmp2!=0.0;
	  nadj = sum(m1 || m2);
	  tmp1=merge(m1,tmp1-s/nadj,tmp1);
	  tmp2=merge(m2,tmp2-s/nadj,tmp2);
	}

      /* pack up to add to interaction */
      mask=tmp1!=0.0;
      ntrue = sum(mask);
      interaction.val <<= pack(tmp1,mask,ntrue);
      interaction.col <<= pack( pcd, mask, ntrue);
      interaction.row <<= ecolab::array<int>(ntrue, tmp1.size());

      mask=tmp2!=0.0;
      ntrue = sum(mask);
      interaction.val <<= pack( tmp2, mask, ntrue);
      interaction.row <<= pack( pcd, mask, ntrue);
      interaction.col <<= ecolab::array<int>(ntrue, tmp2.size());

    }

  /* concatonate new species */
  new_sp = 1;
  density  <<= new_sp;
  sdensity <<= new_sp;
  new_sp = 0;
  create <<=  new_sp;
  activity <<= new_sp;
  species <<= pcoord(new_sp.size())*nprocs + sp_cntr;
  sp_cntr+=new_sp.size()*nprocs;
  
  assert(sum(interaction.diag>=0)==0);
  assert(sum(mutation<0)==0);
}

void shadow_model::lifetimes() 
{ 
  int cell; 
  tclreturn lifetimes; 
  tclcmd cmd;

  for (size_t i=0; i<density.size(); i++) 
    {
      if (create[i]==0 && density[i]>10) 
	create[i] = tstep;
      else if (create[i]>0 && density[i]==0) 
	/* extinction */
	{
	  lifetimes << tstep - create[i] << " ";
	  create[i]=0;
	}
    }
}

extern "C" void dgees_();

void shadow_model::maxeig()
{
  size_t size=interaction.diag.size();
  double *beta=new double[size*size], *eigr=new double[size], 
    *eigi=new double[size], *work=new double[3*size], maxe;
  int lwork=3*size,info,idum,idum1=1;
  tclreturn result;

  for (size_t i=0; i<size; i++)
    for (size_t j=0; j<size; j++)
      beta[i*size + j]=0;

  for (size_t i=0; i<size; i++) 
    beta[i*size+i]=interaction.diag[i];
  for (size_t i=0; i<interaction.val.size(); i++)
    beta[interaction.row[i]*size + interaction.col[i]]=interaction.val[i];

#ifdef LAPACK
  dgees_("N","N",NULL,&size,beta,&size,&idum,eigr,eigi,
	NULL,&idum1,work,&lwork,NULL,&info);
  if (info!=0)
    error("Eigenvalues not converged");
#endif

  maxe=-FLT_MAX;
  for (size_t i=0; i<size; i++)
    maxe = max(maxe, eigr[i]);
  result << maxe;

  delete [] beta; delete [] eigr; delete [] eigi; delete [] work;
}


int shadow_model::newact()
{
  tclvar threshold("newact_thresh");
  ecolab::array<int> m = density>(int)threshold && create==0;
  create[gen_index(m)]=tstep;
  return sum( m );
}

