/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief random generator based on GNUSL implementation
*/
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

namespace ecolab
{

  class urand: public random_gen
  {
    void operator=(urand&);
    CLASSDESC_ACCESS(urand);
  public:
    gsl_rng *gen;
    urand(const gsl_rng_type *descr=gsl_rng_mt19937) {gen=gsl_rng_alloc(descr);}
    ~urand() {gsl_rng_free(gen);}
    void Seed(int s) {gsl_rng_set(gen,s);}
    void seed(TCL_args args) {Seed(args);}
    double rand();
    /* select a different uniform random generator according the GSL's rng
       string interface */
    void set_gen(TCL_args args); 
    void Set_gen(const string& descr)
    {
      static const gsl_rng_type ** rngTypes=gsl_rng_types_setup();
      const gsl_rng_type **g=rngTypes;
      for (; *g; ++g)
        if (descr==(*g)->name)
          {
            gsl_rng_free(gen);
            gen=gsl_rng_alloc(*g);
            break;
          }
      if (!*g) throw error("Cannot create generator %s",descr.c_str());
    }
  };

  class gaussrand: public random_gen
  {
    void operator=(gaussrand&);
    CLASSDESC_ACCESS(gaussrand);
  public:
    urand uni;
    double rand();
  };
}

 /* don't attempt to send these objects over the wire ! */
#ifdef CLASSDESC
#pragma omit pack  ecolab::urand 
#pragma omit unpack  ecolab::urand 
#endif
namespace classdesc_access
{
  template <>
  struct access_pack<ecolab::urand>: 
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <>
  struct access_unpack<ecolab::urand>: 
    public classdesc::NullDescriptor<classdesc::unpack_t> {};
  template <>
  struct access_pack<ecolab::urand*>: 
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <>
  struct access_unpack<ecolab::urand*>: 
    public classdesc::NullDescriptor<classdesc::unpack_t> {};
}
