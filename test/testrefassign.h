/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

using namespace ecolab;
struct Testrefassign: public TCL_obj_t
{
  TCL_obj_ref<random_gen> rng;
  double rand() {return rng->rand();}
};
