/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <ecolab.h>
#include "arrays.h"
using namespace ecolab;
#include "complex_tcl_args.h"
#include "complex_tcl_args.cd"
#include "ecolab_epilogue.h"

foo foobar;
make_model(foobar);

void foo::seta(TCL_args args)
{ args>>a;}
