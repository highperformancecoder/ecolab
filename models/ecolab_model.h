/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <netcomplexity.h>
#include <cairoSurfaceImage.h>
#include <ecolab.h>
using classdesc::Object;

#include <random>
#include <vector>
#include <pack_stl.h>

#ifdef USE_FLOAT
using Float=float;
#else
using Float=double;
#endif

struct ConnectionPlot: public Object<ConnectionPlot, CairoSurface>
{
  array<int> density;
  sparse_mat<Float> connections;
  bool redraw(int x0, int y0, int width, int height) override;
  void requestRedraw() const {if (surface) surface->requestRedraw();}
  void update(const array<int>& d, const sparse_mat<Float>& c) {
    density=d;
    connections=c;
    requestRedraw();
  }
};

struct ModelData
{
  array<int> species;
  array<Float> create;
  array<Float> repro_rate, mutation, migration;
  sparse_mat<Float> interaction;
  sparse_mat_graph foodweb;
  unsigned long long tstep=0, last_mut_tstep=0, last_mig_tstep=0;
  //mutation parameters
  float sp_sep=0.1, mut_max=0, repro_min=0, repro_max=1, odiag_min=0, odiag_max=1;

  void makeConsistent(size_t nsp)
    {
      if (!species.size())
        species=pcoord(nsp);

      if (!create.size()) create.resize(species.size(),0);
      if (!mutation.size()) mutation.resize(species.size(),0);
      if (!migration.size()) migration.resize(species.size(),0);
    }
   
  void random_interaction(unsigned conn, double sigma);

  void condense(const array<bool>& mask, size_t mask_true);
  void mutate(const array<int>&); 

  double complexity() {return ::complexity(foodweb);}
};

/* ecolab cell  */
template <class CellBase>
struct EcolabPoint: public Exclude<CellBase>
{
  Float salt;  /* random no. used for migration */
  array<int,typename CellBase::template Allocator<int>> density{this->template allocator<int>()};
  void generate(unsigned niter, const ModelData&);
  void condense(const array<bool>& mask, size_t mask_true);
  array<int> mutate(const array<double>&);
  unsigned nsp() const; ///< number of living species in this cell
  /// Rounding function, randomly round up or down, in the range 0..INT_MAX
  int ROUND(Float x); 
  template <class E> array<int> RoundArray(const E& x);
  Exclude<std::mt19937> rand; // random number generator
};

// for the panmictic model, we need to use std::allocator
struct AllocatorBase
{
  template <class T> using Allocator=std::allocator<T>;
  template <class T> Allocator<T> allocator() const {return Allocator<T>();}
};

struct PanmicticModel: public ModelData, public EcolabPoint<AllocatorBase>, public ecolab::Model<PanmicticModel>
{
  ConnectionPlot connectionPlot;
  void updateConnectionPlot() {connectionPlot.update(density,interaction);}
  void makeConsistent() {ModelData::makeConsistent(density.size());}
  void seed(unsigned x) {rand.seed(x);}
  void generate(unsigned niter);
  void generate() {generate(1);}
  void condense();
  void mutate();
  array<double> lifetimes();
};

struct EcolabCell: public EcolabPoint<ecolab::CellBase>, public graphcode::Object<EcolabCell> {};

class SpatialModel: public ModelData, public EcolabGraph<EcolabCell>,
                    public ecolab::Model<SpatialModel>
{
  size_t numX=1, numY=1;
  CLASSDESC_ACCESS(SpatialModel);
public:
  size_t makeId(size_t x, size_t y) const {return x%numX + numX*(y%numY);}
  void setGrid(size_t nx, size_t ny);
  EcolabCell& cell(size_t x, size_t y) {
    return *objects[makeId(x,y)];
  }
  array<unsigned> nsp() const;
  void makeConsistent();
  void seed(unsigned x) {forAll([=](EcolabCell& cell){cell.rand.seed(x);});}
  void generate(unsigned niter);
  void generate() {generate(1);}
  void condense();
  void mutate();
  void migrate();
};

