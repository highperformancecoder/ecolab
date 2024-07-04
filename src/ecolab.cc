/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "tcl++.h"
#define TK 1
#include "cairoSurfaceImage.h"
#include "plot.h"
#include "pythonBuffer.h"
#include "object.cd"
#include "ecolab_epilogue.h"
using namespace ecolab;
using namespace std;

#include <dlfcn.h>

namespace ecolab
{
  bool interpExiting=false;
  void interpExitProc(ClientData cd) {}

  const char* TCL_args::str() 
#if (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION==0)
    {return Tcl_GetStringFromObj(pop_arg(),NULL);}
#else
    {return Tcl_GetString(pop_arg());}
#endif

  std::string ecolabHome=ECOLAB_HOME;
  CLASSDESC_ADD_GLOBAL(ecolabHome);
  CLASSDESC_DECLARE_TYPE(Plot);
  CLASSDESC_PYTHON_MODULE(ecolab);
}

// place for initialising any EcoLab extensions to the TCL
// interpreter, to be called by tkinter's Tk() object
extern "C" int Ecolab_Init(Tcl_Interp* interp)
{
  CairoSurface::registerImage();
  return TCL_OK;
}

// some linkers add an _
extern "C" int _Ecolab_Init(Tcl_Interp* interp) {return Ecolab_Init(interp);}

