/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "ecolab--.h"
#include "ecolab--.cd"

ecolab_t ecolab;
make_model(ecolab);

unsigned ecolab_t::generate(TCL_args args)
{
  unsigned t, nstep=args;
  for (t=0; t<nstep; t++, tstep++)
    {n=(n>=1)*n*(1+r-beta*n+gamma.rand()); }
}

