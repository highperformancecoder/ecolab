/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <arrays.h>
#include "ecolab_epilogue.h"
using ecolab::array_ns::array;

int main()
{
  array<double> x(10,1);
  array<size_t> i=pos(array<bool>(x==0));
  i=pos(x==0);
  return 0;
}
