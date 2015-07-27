/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

struct bar
{
  void foo();
};

struct foo
{
  int x[10], y[10][10];
  bar z[10];
  int get_x(TCL_args a);
  int get_y(TCL_args a);
};
