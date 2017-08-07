/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

struct foo1
{
  int bar;
};

struct foo2
{
  int bar;
  foo2(const ecolab::TCL_args& x) {if (x.count>0) bar=x[0];}
};

struct foo
{
  ecolab::TCL_obj_ref<foo1> bar;
};

