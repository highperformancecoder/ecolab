/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
using namespace ecolab;
#include "tcl-arrays.h"
#include "tcl-arrays.cd"
#include "ecolab_epilogue.h"

foo foobar;
make_model(foobar);

int foo::get_x(TCL_args a)
{return x[a.get<int>()];}

int foo::get_y(TCL_args a)
{
  int i=a, j=a;
  return y[i][j];
};

void bar::foo() {tclvar("foo_called")=1;}
