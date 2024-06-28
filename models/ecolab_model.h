/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#if 0 /* for the moment, comment this out as MTL not working with gcc 3 */
#include <mtl/matrix.h>
using mtl::matrix;
#endif

#include <netcomplexity.h>

using GRAPHCODE_NS::object;
using GRAPHCODE_NS::objref;
using GRAPHCODE_NS::omap;
using GRAPHCODE_NS::Wrap;
using classdesc::Object;

/* ecolab cell  */
struct ecolab_point_data
{
  double salt;  /* random no. used for migration */
  array<int> density;
  void generate_point(unsigned niter);
  void condense_point(const array<bool>& mask, unsigned mask_true);
  array<int> mutate_point(const array<double>&); 
};

class ecolab_point: public ecolab_point_data, public Object<ecolab_point,GRAPHCODE_NS::object>
{
public:
  using Object<ecolab_point,GRAPHCODE_NS::object>::pack;
  // override object::pack
  template <class A, class M> 
  A pack(const A& e, const M& mask, long ntrue=-1)
  {return array_ns::pack(e,mask,ntrue);} 
};



inline ecolab_point& cell(objref& o) {return dynamic_cast<ecolab_point&>(*o);}

#include <vector>
#include <pack_stl.h>

class Grid2D: public GRAPHCODE_NS::Graph
{
public:
  size_t xsize, ysize;
  Grid2D() {xsize=ysize=0;}
  void instantiate(unsigned xx, unsigned yy)
  {
    xsize=xx; ysize=yy;
    for (unsigned x=0; x<xsize; x++)
      for (unsigned y=0; y<ysize; y++)
	this->AddObject<ecolab_point>(x+y*xsize).proc=(x*nprocs())/xsize;
    for (int x=0; x<int(xsize); x++)
      for (int y=0; y<int(ysize); y++)
	{
	  /* connect up a von Neumann neighbourhood */
	  objref& w=objects[x+y*xsize];
	  w->push_back(w);
	  w->push_back(objects[Wrap(x+1,int(xsize))+y*xsize]);
	  w->push_back(objects[Wrap(x-1,int(xsize))+y*xsize]);
	  w->push_back(objects[x+(Wrap(y+1,int(ysize))*xsize)]);
	  w->push_back(objects[x+(Wrap(y-1,int(ysize))*xsize)]);
	}
    this->rebuild_local_list();
    this->Distribute_Objects();
  }
};

class ecolab_grid: public ecolab_point_data, protected Grid2D
{
protected:
  /* gain access to Grid2D iterators, not Pin's iterators */
  typedef Grid2D::iterator iterator;
  iterator begin() {return Grid2D::begin();}
  iterator end() {return Grid2D::end();}
public:
  unsigned size() {return Grid2D::size();}

  urand uni;

  void set_grid(unsigned x, unsigned y);
  //void forall(TCL_args args);
  std::string get(unsigned x, unsigned y);
  ecolab_grid() {/*AddObject(*static_cast<object*>(this),0); rebuild_local_list();*/ xsize=ysize=1;}
  void gather();
};

/* Rounding function, randomly round up or down, in the range 0..INT_MAX */
inline int ROUND(double x) 
{
  double dum;
  if (x<0) x=0;
  if (x>INT_MAX-1) x=INT_MAX-1;
  return std::fabs(std::modf(x,&dum)) > array_urand.rand() ?
    (int)x+1 : (int)x;
}

inline int ROUND(float x) {return ROUND(double(x));}

template <class E>
inline array<int> ROUND(const E& x)
{
  array<int> r(x.size());
  for (size_t i=0; i<x.size(); i++)
    r[i]=ROUND(x[i]);
  return r;
}

struct model_data
{
  array<int> species;
  array<double> create;
  array<double> repro_rate, mutation, migration;
  sparse_mat interaction;
  sparse_mat_graph foodweb;
};

class ecolab_model: public ecolab_grid, public model_data
{
  void mutate_model(array<int>); 
public:
  // for some reason, this using declaration does not work with g++,
  // so we need to explicitly qualify create on all usages. Bugger you
  // g++!
  using model_data::create;
  unsigned long long tstep, last_mut_tstep, last_mig_tstep;
  //mutation parameters
  float sp_sep, mut_max, repro_min, repro_max, odiag_min, odiag_max;

  ecolab_model()     
  {repro_min=0; repro_max=1; mut_max=0; odiag_min=0; odiag_max=1;
  tstep=0; last_mut_tstep=0; last_mig_tstep=0;}

  void make_consistent()
    {
      if ((xsize*ysize)%nprocs()!=0) throw error("number of grid cells must be a multiple of the number of execution threads");
      if (!species.size())
	if (size())
	  {
	    species=pcoord(cell(*begin()).density.size());
	    for (iterator i=begin(); i!=end(); i++) 
	      if (cell(*i).density.size()!=species.size())
                throw error("%d:grid needs to initialised with same no. of species at each cell",myid);
	  }
      // bugger you, g++!
      if (!model_data::create.size()) model_data::create.resize(species.size(),0);
      if (!mutation.size()) mutation.resize(species.size(),0);
      if (!migration.size()) migration.resize(species.size(),0);
    }
   
  void distribute_cells();
  void random_interaction(unsigned conn, double sigma);

  void generate(unsigned niter);
  void generate() {generate(1);}
  void condense();
  void mutate();
  array<double> lifetimes();
  void migrate();

  double complexity() {return ::complexity(foodweb);}
};
