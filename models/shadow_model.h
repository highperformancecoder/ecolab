/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* The good old ecolab model that also has a Mark Bedau neutral shadow
   model "joined-at-the-hip". See paper "Ecolab perspective on Bedau
   evolutionary statistics" for more details. */

#include "arrays.h"
#include "sparse_mat.h"

using namespace ecolab;

class shadow_model: public classdesc::TCL_obj_t
{
public:
  int tstep, last_mut_tstep;
  array<int> density, sdensity, species, create;
  array<double> repro_rate, mutation, activity, mig_ns, mig_ew;
  sparse_mat interaction;

  //mutation parameters
  float sp_sep, mut_max, repro_min, repro_max, odiag_min, odiag_max;

  void random_interaction(TCL_args);

  void generate();
  void condense();
  void mutate();
  shadow_model() 
  {repro_min=0; repro_max=1; mut_max=0; odiag_min=0; odiag_max=1;
  tstep=0; last_mut_tstep=0;}
  void make_consistent()  /* perform this routine at tstep==0 */
  {
    if (!species.size()) {species=pcoord(density.size());}
    if (!create.size()) {create.resize(density.size(),tstep);}
    if (!mig_ns.size()||!mig_ew.size()) {
      mig_ew.resize(density.size(),0);
      mig_ns=mig_ew;
    }
    sdensity=density; activity.resize(density.size(),0);
  }
  void lifetimes();
  void maxeig();
  int newact();
};


