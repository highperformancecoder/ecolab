/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "tcl++.h"
#include "ecolab_epilogue.h"

//ecolab::tclvar TCL_obj_lib("ecolab_library",INSTALLED_ECOLAB_LIB);
static int dum=ecolab::setEcoLabLib(INSTALLED_ECOLAB_LIB);
