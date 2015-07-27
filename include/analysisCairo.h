/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ANALYSIS_CAIRO_H
#define ANALYSIS_CAIRO_H
#include "plot.h"

namespace ecolab
{
  struct HistoGram: public Plot, public HistoStats
  {
    HistoGram() {plotType=bar;}
    // overload add_data to plot the data
    void add_data(TCL_args args) {
      HistoStats::add_data(args);
      reread();
    }
    void reread() {
      Plot::clear();
      if (size() && max>min) Plot::add(1,bins(), histogram());
    }
    void clear() {
      Plot::clear();
      HistoStats::clear();
    }
    void outputdat(TCL_args args);
  };
}

#include "analysisCairo.cd"
#endif
