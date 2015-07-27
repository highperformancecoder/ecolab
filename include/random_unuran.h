/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief random generator based on the UNURAN library
*/

#include <unuran.h>

/* dummy declaration to force classdesc to generate a dummy definition */
struct unur_gen; 

namespace ecolab
{
  /// uniform random generator. Uses a new PRNG uni rand if PRNG
  /// available, the default uniform random number generator otherwise
  class urand: public random_gen
  {
    UNUR_URNG *gen;
    friend class gaussrand;
    friend class unuran;
    void operator=(const urand&);
    urand(const urand&);
    CLASSDESC_ACCESS(urand);
  public:
    urand();
    ~urand();
    void seed(int s) {unur_urng_seed(gen,s);}
    void Seed(int s) {seed(s);} // for backwards compatibility
    double rand() {return unur_urng_sample(gen);};
    // for backwards compatibility
    void Set_gen(const char* descr) {set_gen(descr);} 
    /** select a different uniform random generator according the prng's
        string interface. If PRNG not avail, this routine is a nop. */
    void set_gen(const char* descr);
  };

  /// universal non-uniform random generator
  class unuran: public random_gen
  {
    UNUR_GEN   *gen;
    void operator=(unuran&);
    CLASSDESC_ACCESS(unuran);
  public:
    urand uni;
    /* specify a random generator according to unuran's string interface */
    void set_gen(TCL_args args) {Set_gen(args);}
    UNUR_GEN *get_gen() {return gen;}
    void Set_gen(const char *descr)
    {
      if (gen) unur_free(gen);
      gen=unur_str2gen(descr); 
      if (gen==NULL) throw error("Cannot create generator %s",descr);
      unur_chg_urng(gen,uni.gen); 
    }
    unuran(): gen(NULL) {}
    unuran(const char* descr): gen(NULL) {Set_gen(descr);}
    ~unuran() {if (gen) unur_free(gen);}
    double rand();
  };

  /// Gaussian (normal) random generator
  class gaussrand: public unuran
  {
  public:
    gaussrand(): unuran("normal()") {}
  };

}

#ifdef _CLASSDESC
#pragma omit pack ecolab::urand 
#pragma omit unpack ecolab::urand 
#pragma omit pack ecolab::unuran 
#pragma omit unpack ecolab::unuran 
#endif

namespace classdesc_access
{
  namespace cd=classdesc;
  /* don't attempt to send these objects over the wire ! */
  template <> struct access_pack<UNUR_GEN *>:
    public cd::NullDescriptor<cd::pack_t> {};
  template <> struct access_unpack<UNUR_GEN *>:
    public cd::NullDescriptor<cd::unpack_t> {};


  template <> struct access_pack<ecolab::urand>:
    public cd::NullDescriptor<cd::pack_t> {};
  template <> struct access_unpack<ecolab::urand>:
    public cd::NullDescriptor<cd::unpack_t> {};

  template <> struct access_pack<ecolab::unuran>:
    public cd::NullDescriptor<cd::pack_t> {};
  template <> struct access_unpack<ecolab::unuran>:
    public cd::NullDescriptor<cd::unpack_t> {};
}
