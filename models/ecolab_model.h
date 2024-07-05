/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <netcomplexity.h>

using classdesc::Object;

/* ecolab cell  */
struct ecolab_point
{
  double salt;  /* random no. used for migration */
  array<int> density;
  void generate_point(unsigned niter);
  void condense_point(const array<bool>& mask, unsigned mask_true);
  array<int> mutate_point(const array<double>&); 
};

#include <vector>
#include <pack_stl.h>

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
inline array<int> ROUND(const E& x)
{
  array<int> r(x.size());
  for (size_t i=0; i<x.size(); i++)
    r[i]=ROUND(x[i]);
  return r;
}

struct model_data: public ecolab_point
{
  array<int> species;
  array<double> create;
  array<double> repro_rate, mutation, migration;
  sparse_mat interaction;
  sparse_mat_graph foodweb;
};

class ecolab_model: public model_data
{
  void mutate_model(array<int>); 
public:
  // for some reason, this using declaration does not work with g++,
  // so we need to explicitly qualify create on all usages. Bugger you
  // g++!
  using model_data::create;
  unsigned long long tstep, last_mut_tstep, last_mig_tstep;
  //mutation parameters
  float sp_sep, mut_max, repro_min, repro_max, odiag_min, odiag_max;

  ecolab_model()     
  {repro_min=0; repro_max=1; mut_max=0; odiag_min=0; odiag_max=1;
  tstep=0; last_mut_tstep=0; last_mig_tstep=0;}

  void make_consistent()
    {
      if (!species.size())
        species=pcoord(density.size());

      // bugger you, g++!
      if (!model_data::create.size()) model_data::create.resize(species.size(),0);
      if (!mutation.size()) mutation.resize(species.size(),0);
      if (!migration.size()) migration.resize(species.size(),0);
    }
   
  void random_interaction(unsigned conn, double sigma);

  void generate(unsigned niter);
  void generate() {generate(1);}
  void condense();
  void mutate();
  array<double> lifetimes();

  double complexity() {return ::complexity(foodweb);}
};
