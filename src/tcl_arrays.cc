/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "arrays.h"
#include "ecolab_epilogue.h"
using namespace ecolab;
using namespace std;
using array_ns::operator<<;
using array_ns::array;

int ecolab_tcl_arrays_link;

/* set random number seed */

namespace ecolab
{

NEWCMD(srand,1)
{
  ::srand(atoi(argv[1]));
}

/* urand n [max [min]] return a list of uniform random numbers between
   min and max, which default to 0 and 1 respectively.  */

NEWCMD(unirand,1)
{
  array<double> u(atoi(argv[1]));
  double max=(argc>2)? atof(argv[2]):1;
  double min=(argc>3)? atof(argv[3]):0;
  tclreturn r;

  fillrand(u);
  r << u*(max-min) + min;
}

/* grand n [std [mean]] return a list of normally distributed random
   numbers with mean and standard deviation, which default to 0 and 1
   respectively.  */

NEWCMD(grand,1)
{
  array<double> u(atoi(argv[1]));
  double std=(argc>2)? atof(argv[2]):1;
  double mean=(argc>3)? atof(argv[3]):0;
  tclreturn r;

  fillgrand(u);
  r << u*std + mean;
}

/* prand n [std [mean]] return a list of Poisson distributed random
   numbers with mean and standard deviation, which default to 0 and 1
   respectively.  */

NEWCMD(prand,1)
{
  array<double> u(atoi(argv[1]));
  double std=(argc>2)? atof(argv[2]):1;
  double mean=(argc>3)? atof(argv[3]):0;
  tclreturn r;

  fillprand(u);
  r << u*std + mean;
}

/* unique_rand n [max [min]] return a list of unique random
   integers between min and max, which default to 0 and n
   respectively.  */

NEWCMD(unique_rand,1)
{
  array<int> u(atoi(argv[1]));
  double max=(argc>2)? atoi(argv[2]):atoi(argv[1]);
  double min=(argc>3)? atoi(argv[3]):0;
  tclreturn r;

  fill_unique_rand(u,max);
  r << u + min;
}

NEWCMD(constant,2)
{
  array<double> u(atoi(argv[1]));
  tclreturn r;
  u=atof(argv[2]);
  r << u;
}

NEWCMD(pcoord,1)
{
  tclreturn r;
  r<<array_ns::pcoord(atoi(argv[1]));
}

/* Bunch of reduction functions */

NEWCMD(max,1)
{
  int i,n;
  CONST84 char **v;
  double m=-DBL_MAX;
  tclreturn result;
  if (Tcl_SplitList(interp(),const_cast<char*>(argv[1]),&n,&v)==TCL_OK)
    for (i=0; i<n; i++) m=std::max(m,atof(v[i]));
  Tcl_Free((char*)v);
  result << m;
} 


NEWCMD(min,1)
{
  int i,n;
  CONST84 char **v;
  double m=DBL_MAX;
  tclreturn result;
  if (Tcl_SplitList(interp(),const_cast<char*>(argv[1]),&n,&v)==TCL_OK)
    for (i=0; i<n; i++) m=std::min(m,atof(v[i]));
  Tcl_Free((char*)v);
  result << m;
} 


NEWCMD(av,1)
{
  int i,n;
  CONST84 char **v;
  double m=0;
  tclreturn result;
  if (Tcl_SplitList(interp(),const_cast<char*>(argv[1]),&n,&v)==TCL_OK)
    for (i=0; i<n; i++) m+=atof(v[i]);
  Tcl_Free((char*)v);
  result << m/n;
} 

}
