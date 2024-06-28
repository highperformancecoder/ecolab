/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "tcl++.h"
#define TK 1
#include "cairoSurfaceImage.h"
#include "ecolab_epilogue.h"
using namespace ecolab;

//ecolab::tclvar TCL_obj_lib("ecolab_library",INSTALLED_ECOLAB_LIB);
//static int dum=ecolab::setEcoLabLib(INSTALLED_ECOLAB_LIB);

// place for initialising any EcoLab extensions to the TCL
// interpreter, to be called by tkinter's Tk() object
int Ecolab_Init(Tcl_Interp* interp)
{
  CairoSurface::registerImage();
  return TCL_OK;
}
