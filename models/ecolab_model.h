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

struct ConnectionPlotBase: public CairoSurface, public classdesc::object {};

struct ConnectionPlot: public Object<ConnectionPlot, ConnectionPlotBase>
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
#ifdef SYCL_LANGUAGE_VERSION
  template <class T> using Allocator=HostSharedAllocator<T>;
#else
  template <class T> using Allocator=std::allocator<T>;
#endif

  array<int,Allocator<int>> species;
  array<Float,Allocator<Float>> create, repro_rate, mutation, migration;
  sparse_mat<Float,Allocator> interaction;
  using UnsignedArray=array<unsigned,Allocator<unsigned>>;
  std::vector<UnsignedArray,Allocator<UnsignedArray>> oDiagIdx;
  sparse_mat_graph foodweb;
  unsigned long long tstep=0, last_mut_tstep=0, last_mig_tstep=0;
  //mutation parameters
  Float sp_sep=0.1, mut_max=0, repro_min=0, repro_max=1, odiag_min=0, odiag_max=1, gen_bias=0;
  /// flags to disable mutation of mutation and migration rates
  bool fixMutation=false, fixMigration=false;
  
  void makeConsistent(size_t nsp);
  void computeODiagIdx();
  void random_interaction(unsigned conn, double sigma);
  void condense(const array<bool>& mask, size_t mask_true);
  void mutate(const array<int>&); 
  double complexity() {return ::complexity(foodweb);}
  /// May criterion connectivity σ²
  Float connectivity() const;
};

template <class E, class P> struct RoundArray; 

template <class T,class A> void setArray(array<T,A>&, const array<T>&);
template <class T,class A> array<T> getArray(const array<T,A>&);

/* ecolab cell  */
class EcolabPoint
{
public:
#ifdef SYCL_LANGUAGE_VERSION
  template <class T> using Allocator=GlobalDeviceAllocator<T>;
#else
  template <class T> using Allocator=std::allocator<T>;
#endif
  using UnsignedArray=array<unsigned,Allocator<unsigned>>;
  using LocalArray=array<unsigned,LocalAllocator<unsigned>>;

  Float salt;  /* random no. used for migration */
  array<int,Allocator<int>> density;
  
  void generate(unsigned niter, const ModelData&);
  void condense(const array<bool>& mask, size_t mask_true);
  template <class E> LocalArray mutate(const E&);
  unsigned nsp() const; ///< number of living species in this cell
  /// Rounding function, randomly round up or down, in the range 0..INT_MAX
  int ROUND(Float x);
  template <class E> RoundArray<E,EcolabPoint> roundArray(const E& expr);
#ifdef SYCL_LANGUAGE_VERSION
  Exclude<SyclRandomEngine<std::mt19937,USMAlloc::shared>> rand
    {syclQ().get_device().get_info<sycl::info::device::max_work_group_size>()}; // TODO make configurable?
#else
  Exclude<std::mt19937> rand; // random number generator
#endif
};

struct PanmicticModel: public ModelData, public EcolabPoint, public ecolab::Model<PanmicticModel>
{
  ConnectionPlot connectionPlot;
  void updateConnectionPlot() {connectionPlot.update(this->density,interaction);}
  void makeConsistent() {ModelData::makeConsistent(this->density.size());}
  void seed(unsigned x) {rand.seed(x);}
  void generate(unsigned niter);
  void generate() {generate(1);}
  /// returns number of extinctions
  unsigned condense();
  void mutate();
  array<Float> lifetimes();
  /// returns the connectivity of species added in after \a beforeNsp
  Float mutantConnectivity(size_t beforeNsp) const;
  /// returns connectivity of all species that have gone extinct
  Float extinctionConnectivity() const;
};

struct EcolabCell: public EcolabPoint, public graphcode::Object<EcolabCell>
{
  unsigned id=0; // stash the graphcode node id here so it is accessible from within the cell, rather than its pointer wrapper
};

class SpatialModel: public ModelData, public EcolabGraph<EcolabCell>,
                    public ecolab::Model<SpatialModel>
{
  size_t numX=1, numY=1;
  CLASSDESC_ACCESS(SpatialModel);
  size_t maxNbrs=0;
public:
  static constexpr size_t log2MaxNsp=11;
  // function valid for x∈(-numX,∞], y∈(-numY,∞]
  size_t makeId(size_t x, size_t y) const {return (x+numX)%numX + numX*((y+numY)%numY);}
  void setGrid(size_t nx, size_t ny);
  EcolabCell& cell(size_t x, size_t y) {
    return *objects[makeId(x,y)];
  }
  array<unsigned> nsp() const;
  void makeConsistent();
  void seed(unsigned x) {forAll([=](EcolabCell& cell,size_t){cell.rand.seed(x);});}
  void generate(unsigned niter);
  void generate() {generate(1);}
  /// returns number of extinctions
  unsigned condense();
  void mutate();
  unsigned migrate();
};

