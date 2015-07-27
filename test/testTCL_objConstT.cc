/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
using namespace ecolab;

#include "testTCL_objConstT.h"
#include "testTCL_objConstT.cd"
#include "ecolab_epilogue.h"
const string Foo::staticBar="hello";

const std::string Foo::staticFun()
{return staticBar;}

Foo foo;
make_model(foo);

const Foo cfoo;
make_model(cfoo);
