/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief Miscellaneous BLT based utilities
*/
#ifndef ANALYSISBLT_H
#define ANALYSISBLT_H
#include "arrays.h"

namespace ecolab
{

#ifdef BLT
#if BLT_MAJOR_VERSION >= 3
#include <bltVector.h>
#endif

class histogram_data
{
  Blt_Vector *x, *y;
  string name;  //store namespace name here for later reference
  CLASSDESC_ACCESS(histogram_data);
public:
  HistoStats data;
  /** legacy member references */
  unsigned& nbins;
  bool& xlogison;
  float &min, &max;
  histogram_data(): nbins(data.nbins), xlogison(data.logbins),
                   min(data.min), max(data.max) {}
  void init_BLT_vects(TCL_args args);
  void add_data(TCL_args args);
  void clear();
  void reread();
  void outputdat(TCL_args args);
};

#ifdef _CLASSDESC
#pragma omit pack histogram_data
#pragma omit unpack histogram_data
#endif

#endif

}

#include "analysisBLT.cd"
#endif
