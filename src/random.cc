/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "ecolab_epilogue.h"
using namespace ecolab;
using std::vector;

extern "C" void ecolab_random_link() {}

double affinerand::rand() {return scale*gen->rand()+offset;}
void affinerand::set_gen(TCL_args args) 
{
  char *name=args;
  if (TCL_obj_properties().count(name)==0)
    throw error("%s does not exist!",name);
  if (random_gen *gen=TCL_obj_properties()[name]->memberPtrCasted<random_gen>())
    Set_gen(gen);
  else
    throw error("%s has incorrect argument type",name); 
}

#ifdef UNURAN
#ifdef PRNG
#include <unuran_urng_prng.h>
urand::urand(): gen(unur_urng_prng_new("mt19937(19863)")) {seed(1);}
void urand::set_gen(const char* descr)
{
  unur_urng_free(gen); gen=unur_urng_prng_new(descr);
  if (gen==NULL) throw error("Cannot create generator %s",descr);
}
urand:: ~urand() {unur_urng_free(gen);}

#else
urand::urand(): gen(unur_get_default_urng()) {}
void urand::set_gen(const char* descr) {}
urand:: ~urand() {}

#endif

double unuran::rand() 
{
  if (gen) 
    switch (unur_distr_get_type(unur_get_distr (gen)))
      {
      case UNUR_DISTR_CONT: 
      case UNUR_DISTR_CEMP: return unur_sample_cont(gen);
      case UNUR_DISTR_CVEC:
      case UNUR_DISTR_CVEMP: throw error("Multivariate distributions not supported");
      case UNUR_DISTR_DISCR: return unur_sample_discr(gen);
      default: throw error("unknown type of distribution");
      }
  else
    throw error("generator not initialised");
}

#elif defined(GNUSL)
double urand::rand() {return gsl_rng_uniform(gen);}
void urand::set_gen(TCL_args args)  {Set_gen(args);}
double gaussrand::rand() {return gsl_ran_gaussian(uni.gen,1.0);}
#else

double urand::rand() {return ((double)::rand())/RAND_MAX;}

double gaussrand::rand()
{  
  sum += uni.rand(); 
  n++;
  return (sum - .5*n) * sqrt( 12./n);
}
#endif


static eco_strstream tclfun;
static double tdist(double x)
{
  tclcmd cmd;
  cmd << tclfun << x << "\n";
  return atof(cmd.result.c_str());
}

void distrand::Init(int argc, char *argv[])
{
  if (argc < 2) throw error("no function supplied to %s",argv[0]);
  tclfun << argv[1];
  init(tdist);
}

/* Technique explained in Abramowitz and Stegun, S26.8.2 - to base 16
   rather than base 10 */


void distrand::init(double (*f)(double))
{
  int i,j,k,l;
  double binsz=(max-min)/nsamp, sump;
  vector<double> p(nsamp);

  /* initialize point probability array */
  for (sump=i=0; i<nsamp; i++)
    {
      if ( (p[i] = f( (i+.5)*binsz + min)) >= 0)
	  sump += p[i];
      else
	throw error("distribution negative!");
    }
  for (i=0; i<nsamp; i++) p[i]/=sump;   /* renormalize the p array */

  /* set up the threshold arrays */
  P.resize(width+1);
  PP.resize(width+1);
  pbase.resize(width+1);
  for (i=0; i<=width; i++) pbase[i]=pow(base,i);
  P[0]=0; PP[0]=0; Pwidth=width;
  for (i=1; i<=width; i++)
    {
      P[i]=0; PP[i]=0;
      for (j=0; j<nsamp; j++)
	{
	  P[i]+=delta(p[j],i);
	}	  
      PP[i]=P[i]+PP[i-1];
      P[i]=P[i-1] + pow(base,-i)*P[i];
    }

  /* set up the lookup table */
  a.resize(PP[width]);

  for (i=0; i<width; i++)
    for (j=0, l=PP[i]; j<nsamp; j++)
      for (k=0; k<delta(p[j],i+1); l++,k++)
	a[l]=(j+.5)*(max-min)/nsamp + min;

}

double distrand::rand()
{
  double u=uniform.rand();
  int i;

  while (u>=P[Pwidth]) u=uniform.rand(); /* reject values too big */

  if (P.empty()) throw error("distrand not initialized");

  for (i=0; i<Pwidth; i++)
    if (u>=P[i] && u<P[i+1]) break;
  
  return a[trunctowidth(u-P[i],i+1)+PP[i]];
}

TCLPOLYTYPE(urand, random_gen)
TCLPOLYTYPE(affinerand, random_gen)
TCLPOLYTYPE(gaussrand, random_gen)
TCLPOLYTYPE(distrand, random_gen)

#ifdef UNURAN
TCLPOLYTYPE(unuran, random_gen)
#endif
