/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

struct ecolab_t
{
  double n, r, beta;
  unsigned tstep;
  affinerand gamma;
  unsigned generate(TCL_args);
};
