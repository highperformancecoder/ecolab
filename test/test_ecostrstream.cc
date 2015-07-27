/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "eco_strstream.h"
#include "ecolab_epilogue.h"
#include <iostream>
using namespace std;
using namespace ecolab;
int main()
{
    eco_strstream x;
    x << "hello to" << 'a' << "world" << 1234567890 << 4.56123 |'\n' ;
    x | "var"|123;
    cout <<x <<endl;
    return 0;
}

