/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <netcomplexity.h>
#include <cairoSurfaceImage.h>
#include <graphcode.h>
using classdesc::Object;

#include <vector>
#include <pack_stl.h>

struct ConnectionPlot: public Object<ConnectionPlot, CairoSurface>
{
  array<int> density;
  sparse_mat connections;
  bool redraw(int x0, int y0, int width, int height) override;
  void requestRedraw() const {if (surface) surface->requestRedraw();}
};

struct ModelData
{
  array<int> species;
  array<double> create;
  array<double> repro_rate, mutation, migration;
  sparse_mat interaction;
  sparse_mat_graph foodweb;
  unsigned long long tstep=0, last_mut_tstep=0, last_mig_tstep=0;
  //mutation parameters
  float sp_sep=0.1, mut_max=0, repro_min=0, repro_max=1, odiag_min=0, odiag_max=1;

  void makeConsistent(size_t nsp)
    {
      if (!species.size())
        species=pcoord(nsp);

      // bugger you, g++!
      if (!/*ModelData::*/create.size()) /*ModelData::*/create.resize(species.size(),0);
      if (!mutation.size()) mutation.resize(species.size(),0);
      if (!migration.size()) migration.resize(species.size(),0);
    }
   
  void random_interaction(unsigned conn, double sigma);

  void condense(const array<bool>& mask, size_t mask_true);
  void mutate(const array<int>&); 

  double complexity() {return ::complexity(foodweb);}
};

/* ecolab cell  */
struct EcolabPoint
{
  double salt;  /* random no. used for migration */
  array<int> density;
  void generate(unsigned niter, const ModelData&);
  void condense(const array<bool>& mask, size_t mask_true);
  array<int> mutate(const array<double>&); 
};

struct PanmicticModel: public ModelData, public EcolabPoint
{
  ConnectionPlot connectionPlot;
  void updateConnectionPlot() {
    connectionPlot.density=density;
    connectionPlot.connections=interaction;
    connectionPlot.requestRedraw();
  }
  void makeConsistent() {ModelData::makeConsistent(density.size());}
  void generate(unsigned niter);
  void generate() {generate(1);}
  void condense();
  void mutate();
  array<double> lifetimes();
};

struct EcoLabCell: public EcolabPoint, public graphcode::Object<EcoLabCell> {};

struct SpatialModel: public ModelData, public graphcode::Graph<EcoLabCell>
{
};

