/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief Miscellaneous statistical and GUI based utilities
*/
#ifndef ANALYSIS_H
#define ANALYSIS_H
#include "arrays.h"
#include "graph.h"
#include <vector>
#include <string>
#include <algorithm>

namespace ecolab
{

  /// A class for accessing the palette TCL variable, which is a list of colours
  class palette_class
  { 
  public:
    static int size;
    static std::vector<std::string> table;
    const char * operator[](int i){return table[i%size].c_str();}
    palette_class();
  };

  /// elementary statistics
  struct Stats: public array_ns::array<float>
  {
    double sum, sumsq; ///< \f$\sum x_i, \sum x_i^2\f$
    float max, min;    ///< max and min value
    Stats(): sum(0), sumsq(0), max(-std::numeric_limits<float>::max()), 
             min(std::numeric_limits<float>::max()) {}
    void clear() {
      resize(0); sum=sumsq=0; 
      max=-std::numeric_limits<float>::max();
      min=std::numeric_limits<float>::max();
    }
    double av() {return sum/size();} ///<average
    double median() {
      std::nth_element(begin(),begin()+size()/2,end());
      return *(begin()+size()/2);
    }
    /// standard deviation
    double stddev() {double d=sumsq - sum*sum/size();
      if (d<0) return 0; // deal with roundoff error
      else return sqrt( d/size());}

    /// append an element to the data
    Stats& operator<<=(float x);
    /// append an array of elements to the data
    Stats& operator<<=(const array_ns::array<float>& x);
    /// append an array of elements to the data
    Stats& operator<<=(const array_ns::array<double>& x)
    {operator<<=(array_ns::array<float>(x)); return *this;}

    bool operator<(const Stats& x) const 
    {return std::lexicographical_compare(begin(),end(),x.begin(),x.end());}
  };

  template<class charT, class Traits>
  std::basic_ostream<charT,Traits>& operator<<
    (std::basic_ostream<charT,Traits>& o, const Stats& x)
  {return o<<static_cast<const array_ns::array<float>&>(x);}

  /// Histogramming tool
  struct HistoStats: public Stats
  {
    unsigned nbins; ///< number of bins
    bool logbins;  ///< bin logarithmically
    float logmin; ///< log of minimum postive data element. Set in reread.
    HistoStats(): nbins(100), logbins(false) {}
    array_ns::array<double> histogram(); ///< return histogram
    array_ns::array<double> bins();  ///< return bin lower bounds
    /// return log likelihood ratio of two distributions
    /** @param distribution1 eg lognormal(100,20)
        @param distribution2 eg powerlaw(-1.5)
        @param \f$x_\mathrm{min}\f$
    */
    //double loglikelihood(TCL_args args); 
    /** \brief fit \f$x^{-a}\$

        @returns \f$a\f$ and \f$x_\mathrm{min}\f$
        @param \f$x_\mathrm{min}\f$ (optional) 
    */
    //array_ns::array<double> fitPowerLaw(TCL_args); 

    /// fit \f$\exp(-x/a)\f$ - return \f$a\f$ 
    double fitExponential() {return av();} 

    /// fit \f$\exp(-(x-m)^2/2s\f$, return \f$m\f$, \f$s\f$
    array_ns::array<double> fitNormal() { 
      return array_ns::array<double>()<<av()<<stddev();
    }

    /// fit \f$\exp(-(\ln(x)-\ln m)^2/2s\f$, return \f$m\f$, \f$s\f$
    array_ns::array<double> fitLogNormal();   
  };

  struct NetworkFromTimeSeries: public std::vector<Stats>
  {
    /// network constructed from the statistics 
    ConcreteGraph<DiGraph> net;
    /// number of cells along each dimension of the timeseries
    std::vector<unsigned> n;
    /// construct the network. Preconditions:
    /// resolution.size()==size(), and all elements of this are the
    /// same size
    void constructNet();
    /// columnIdx for loadData. If empty, then columns 1-size() are used.
    std::vector<int> columnIdx;
    /// convenience routine to load data into the vector<Stats>
    void loadData(const char* filename);
  };
}
#include "analysis.cd"

#ifdef BLT
#include "analysisBLT.h"
#endif
#ifdef CAIRO
#include "analysisCairo.h"
#endif
#endif
