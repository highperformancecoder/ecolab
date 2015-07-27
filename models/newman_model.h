/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

using namespace ecolab;

class newman_model: public classdesc::TCL_obj_t
{
public:
  int sp_cntr;    /* used to label newly created species */
  int tstep, last_mut_tstep;
  double mutation;
  array<int> density, species, create;
  array<double> threshold;
  random_gen *pstress;
  void step();
  void condense();
  array<int> lifetimes();
  void make_consistent()  /* perform this routine at tstep==0 */
  {
    if (!species.size()) {species=pcoord(density.size());}
    if (!create.size()) {create.resize(density.size(),tstep);}
    sp_cntr = max(species)+1;   
  }
};

