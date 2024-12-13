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
  array<int,graphcode::Allocator<int>> density;
  sparse_mat<Float, graphcode::Allocator> connections;
  bool redraw(int x0, int y0, int width, int height) override;
  void requestRedraw() const {if (surface) surface->requestRedraw();}
  void update(const array<int,graphcode::Allocator<int>>& d, const sparse_mat<Float, graphcode::Allocator>& c) {
    density=d;
    connections=c;
    requestRedraw();
  }
};

struct ModelData
{
  using FAlloc=graphcode::Allocator<Float>;
  array<int,graphcode::Allocator<int>> species;
  array<Float,FAlloc> create;
  array<Float,FAlloc> repro_rate, mutation, migration;
  sparse_mat<Float,graphcode::Allocator> interaction;
  sparse_mat_graph foodweb;
  unsigned long long tstep=0, last_mut_tstep=0, last_mig_tstep=0;
  //mutation parameters
  float sp_sep=0.1, mut_max=0, repro_min=0, repro_max=1, odiag_min=0, odiag_max=1;

  void makeConsistent(size_t nsp);
  void random_interaction(unsigned conn, double sigma);
  void condense(const array<bool>& mask, size_t mask_true);
  void mutate(const array<int>&); 
  double complexity() {return ::complexity(foodweb);}
};

template <class E, class P> struct RoundArray; 

template <class T,class A> void setArray(array<T,A>&, const array<T>&);
template <class T,class A> array<T> getArray(const array<T,A>&);

/* ecolab cell  */
template <class CellBase>
class EcolabPoint: public Exclude<CellBase>
{
public:
  Float salt;  /* random no. used for migration */
  template <class T> using Allocator=typename CellBase::template CellAllocator<T>;
  array<int,Allocator<int>> density{this->template allocator<int>()};
  
  void generate(unsigned niter, const ModelData&);
  void condense(const array<bool>& mask, size_t mask_true);
  template <class E>
  array<unsigned,Allocator<unsigned>> mutate(const E&);
  unsigned nsp() const; ///< number of living species in this cell
  /// Rounding function, randomly round up or down, in the range 0..INT_MAX
  int ROUND(Float x);
  template <class E> RoundArray<E,EcolabPoint> roundArray(const E& expr);
  Exclude<std::mt19937> rand; // random number generator
};

// for the panmictic model, we need to use std::allocator
struct AllocatorBase
{
  template <class T> using CellAllocator=std::allocator<T>;
  template <class T> CellAllocator<T> allocator() const {return CellAllocator<T>();}
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

struct EcolabCell: public EcolabPoint<ecolab::CellBase>, public graphcode::Object<EcolabCell>
{
  unsigned id=0; // stash the graphcode node id here
};

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
  /// on GPUs, set the 
  void setDensitiesShared();
  void setDensitiesDevice();
  array<unsigned> nsp() const;
  void makeConsistent();
  void seed(unsigned x) {forAll([=](EcolabCell& cell){cell.rand.seed(x);});}
  void generate(unsigned niter);
  void generate() {generate(1);}
  void condense();
  void mutate();
  void migrate();
};

