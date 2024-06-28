/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

///\file
#ifndef RANDOM_H
#define RANDOM_H

#ifdef MPI_SUPPORT
#include <mpi.h>
#endif

#include <string>

#include "classdesc_access.h"
#include "pack_base.h"
#include "pack_stl.h"
#include "pack_graph.h"

#include <stdlib.h>
#include <math.h>

namespace ecolab
{

  /// abstract base class for representing random number generators
  class random_gen
  {
  public:
    virtual double rand()=0;
    virtual ~random_gen() {}
  };
}


#ifdef UNURAN
#include "random_unuran.h"
#elif defined(GNUSL)
#include "random_gsl.h"
#else
#include "random_basic.h"
#endif

namespace ecolab
{
  /// scale and translate a random number generator
  class affinerand: public random_gen
  {
    random_gen *gen;
    bool allocated;
    CLASSDESC_ACCESS(affinerand);
    void del_gen() {if (allocated) delete gen;}
  public:
    double scale, offset;
    affinerand(double s=1, double o=0):  
      scale(s), offset(o) {gen=new urand; allocated=true;}
    affinerand(double s, double o, random_gen *g):  
      scale(s), offset(o) {allocated=false; Set_gen(g);}
    ~affinerand() {del_gen();}
    void Set_gen(random_gen *g) {del_gen(); gen=g; allocated=false;}
    template <class T> void new_gen(const T& g) 
    {del_gen(); gen=new T; allocated=true;}
    double rand();
  };

  /** \brief arbitrary distribution generator (Marsaglia method). 

      Deprecated in favour of unuran.
  */
  class distrand: public random_gen
  {
    std::vector<double> P;
    std::vector<int> PP;
    std::vector<double> a;
    int Pwidth;
    std::vector<double> pbase;
    int base;
    int delta(double x, int i) {return int(pbase[i]*x) % base;}
    int trunctowidth(double x,int w) {return int(pbase[w]*x);}
    urand uniform;
    /* ith hexadecimal digit of x */
    double pow(int x, int y) {return ::pow((double)x,y);}
    CLASSDESC_ACCESS(distrand);
  public:
    int nsamp;  /* no. of sample points in distribution */
    int width;  /* digits of precision (base 16) used from prob. distribution */
    double min, max;  /* distribution endpoints */
    distrand():base(10) {nsamp=10; width=3; min=0; max=1;}
    //void Init(int argc, char *argv[]);
    // fp syntax problematic
    //void init(double (*f)(double));
    void init(std::function<double(double)>);
    double rand();
  };

} // namespace ecolab

inline void pack(classdesc::pack_t& t, const classdesc::string& d, 
          ecolab::random_gen*& a) {}
inline void unpack(classdesc::unpack_t& t, const classdesc::string& d, 
                   ecolab::random_gen*& a) {}

#include "random.cd"

#ifdef UNURAN
#include "random_unuran.cd"
#elif defined(GNUSL)
#include "random_gsl.cd"
#else
#include "random_basic.cd"
#endif

#endif /* RANDOM_H */
