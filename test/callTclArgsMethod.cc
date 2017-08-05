/*
  @copyright Russell Standish 2017
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <TCL_obj_base.h>
#include <ecolab_epilogue.h>
#include <iostream>
using namespace ecolab;
using namespace std;

struct Foo
{
  void bar(const TCL_args& x) {
    cout << x[0].str() << " "<<x[1].str() << endl;
  }
};

int main() {
  Foo foo;
  foo.bar(TCL_args()<<1<<"hello");
}

