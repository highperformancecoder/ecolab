/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "arrays.h"
using namespace ecolab;
using array_ns::array;
using array_ns::pack;
using array_ns::pcoord;
#include "sparse_mat.h" 

#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#include "ecolab_model.h"
#include "pythonBuffer.h"
#include "plot.h"

#include "object.cd"
#include "ecolab_model.cd"

#include "ecolab_epilogue.h"

// TODO - move this into main library
namespace
{
  int addEcoLabPath()
  {
    if (auto path=PySys_GetObject("path"))
      PyList_Append(path,PyUnicode_FromString(ECOLAB_HOME"/lib"));
    return 0;
  }
  int setPath=addEcoLabPath();

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

}


namespace model
{
  PanmicticModel panmictic_ecolab;
  CLASSDESC_ADD_GLOBAL(panmictic_ecolab);
  SpatialModel spatial_ecolab;
  CLASSDESC_ADD_GLOBAL(spatial_ecolab);
  CLASSDESC_PYTHON_MODULE(ecolab_model);
}

void ModelData::random_interaction(unsigned conn, double sigma)
{
  interaction.init_rand(conn,sigma);  
  foodweb = interaction;
}

void PanmicticModel::generate(unsigned niter)
{
  //parallel(args);
  if (tstep==0) makeConsistent();
  EcolabPoint::generate(niter,*this);
  tstep+=niter;
} 

void EcolabPoint::generate(unsigned niter, const ModelData& model)
{ 
  array<double> n(density);
  for (unsigned i=0; i<niter; i++)
    {n += (model.repro_rate + model.interaction*n) * n ;}
  density = ROUND(n);
}


void EcolabPoint::condense(const array<bool>& mask, size_t mask_true)
{
  density = pack( density, mask, mask_true); 
}

void ModelData::condense(const array<bool>& mask, size_t mask_true)   /* remove extinct species */
{
  auto map = enumerate( mask );
  array<bool> mask_off = mask[ interaction.row ] && mask[ interaction.col ];

  create = pack(create, mask, mask_true); 
  species = pack(species, mask, mask_true); 
  repro_rate = pack(repro_rate, mask, mask_true); 
  mutation = pack(mutation, mask, mask_true); 
  migration = pack(migration, mask, mask_true);
  interaction.diag = pack(interaction.diag, mask, mask_true);

  auto mask_off_true=sum(mask_off);

  interaction.val = pack(interaction.val, mask_off, mask_off_true); 
  interaction.row = map[pack(interaction.row, mask_off,mask_off_true)];
  interaction.col = map[pack(interaction.col, mask_off,mask_off_true)];

  foodweb = interaction;
}

void PanmicticModel::condense()
{
  auto mask=density != 0;
  size_t mask_true=sum(mask);
  if (mask.size()==mask_true) return; /* no change ! */
  ModelData::condense(mask,mask_true);
  EcolabPoint::condense(mask, mask_true);
}



/* 
   Mutate operator 
*/


void PanmicticModel::mutate()
{
  array<double> mut_scale(sp_sep * repro_rate * mutation * (tstep - last_mut_tstep));
  last_mut_tstep=tstep;
  array<unsigned> new_sp;
  new_sp <<= EcolabPoint::mutate(mut_scale);
  ModelData::mutate(new_sp);
  density<<=array<int>(new_sp.size(),1);
}

array<int> EcolabPoint::mutate(const array<double>& mut_scale)
{
  array<int> speciations;
  /* calculate the number of mutants each species produces */
  speciations = ROUND(mut_scale * density); 

  /* generate index list of old species that mutate to the new */
  array<int> new_sp = gen_index(speciations); 

  if (new_sp.size()>0) 
    /* adjust density by mutant values i.e. consider that some organisms
       born in previous step are now discovered to be mutants */
    density-=speciations;
  return new_sp;
}


/* do the offdiagonal mutations */
void do_row_or_col(array<double>& tmp, double range, double minval, double gdist)
{
  double r;
  unsigned ntrue;
  int j, pos;
  //tclvar gen_bias("generalization_bias"); /* gen \in [-1,1] */
  double gen= /*exists(gen_bias)? (double)gen_bias:*/0.0;

  /* create or delete some connections */
  r = (2.0*rand())/RAND_MAX - 1+gen;
  r = (r>0)? r/(1+gen): r/(1-gen); 
  if (r!=0) ntrue=(int)(1/fabs(r))-1;
  else ntrue = tmp.size();

  if (ntrue>tmp.size()) ntrue=tmp.size();

  if (r>0)
    for (unsigned j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size()-1) * ((float) rand()/RAND_MAX) +.5);
	if (tmp[pos]==0.0)
	  tmp[pos] = range *((float) rand()/RAND_MAX) + minval;
      }
  else
    for (unsigned j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size()-1) * ((float) rand()/RAND_MAX) +.5);
	tmp[pos]=0;
      }

  /* mutate values */
  array<double> diff(tmp.size());
  diff=merge(tmp!=0.0,range*gdist,0.0);
  gspread(tmp,diff);
}  

/* only run on processor 0 */
void ModelData::mutate(const array<int>& new_sp)
{


  static int sp_cntr=-1;    /* used to label newly created species */
  if (sp_cntr==-1)
    sp_cntr=max(species)+1;

  /* calculate the genetic distances for the mutants from the parents*/
  array<double> gdist(new_sp.size());
  fillprand(gdist);
  gdist *= mutation[new_sp];

  /* 
     change phenotypes randomly, according a normal distribution 
     with std dev gdist 
  */

  array<double> new_repro_rate(repro_rate[new_sp]);
  double range = repro_max - repro_min;
  gspread( new_repro_rate, range*gdist );
  repro_rate <<= new_repro_rate;

  array<double> new_mutation(mutation[new_sp]);
  lgspread( new_mutation, gdist );
  /* limit mutation rate to mut_max */
  mutation <<=  merge( new_mutation < mut_max, new_mutation, mut_max);

  array<double> new_migration(migration[new_sp]);
  lgspread( new_migration, gdist );
  /* limit mutation rate to mut_max */
  migration <<=  new_migration;

  array<double> new_idiag(interaction.diag[new_sp]);
  lgspread( new_idiag, gdist );
  /* limit idiag to avoid it vanishing (causes system instability) - a
     reasonable is to chose it so that the equilibrium value of
     density is always less than half of INT_MAX */
  array<double> max_idiag( -abs(new_repro_rate)/(0.1*INT_MAX) );
  interaction.diag <<= merge( new_idiag < max_idiag, new_idiag, max_idiag);

  /* vary offdiagonal elements */
  range =  odiag_max-odiag_min;

  /* collect interaction data */
  for (size_t i=0; i<new_sp.size(); i++)
    {
      array<double> tmp1(species.size()+i), tmp2(species.size()+i);
      double s;
      tmp1 = 0; tmp2=0;
      array<int> pcd = pcoord(tmp1.size());
	      
      /* project out connections for row[new_sp[i]] */
      array<int> mask( interaction.row==unsigned(new_sp[i]) );
      int ntrue = sum(mask);
      tmp1[ pack(interaction.col,mask,ntrue) ] = 
	pack(interaction.val,mask,ntrue);
      tmp1[ new_sp[i] ] = interaction.diag[new_sp[i]];

      do_row_or_col(tmp1,range,odiag_min,gdist[i]);
      
      /* project out connections for col[new_sp[i]] */
      mask = interaction.col==unsigned(new_sp[i]);
      ntrue = sum(mask);
      tmp2[ pack( interaction.row, mask, ntrue) ] = 
	pack( interaction.val, mask, ntrue);
      tmp2[ new_sp[i] ] = interaction.diag[new_sp[i]];

      do_row_or_col(tmp2,range,odiag_min,gdist[i]);

      /* adjust offdiag vals so that n.\beta n<0 for all positive n */
      /* construct list of offdiag vals that sum positively */
      mask = tmp1+tmp2>0;
      s=sum(tmp1+tmp2,mask)+interaction.diag[i];
      if (s>0)
	{
	  int nadj;
	  array<int> m1( mask && tmp1!=0.0 );
	  array<int> m2( mask && tmp2!=0.0 );
	  nadj = sum(m1 || m2);
	  tmp1=merge(m1,tmp1-s/nadj,tmp1);
	  tmp2=merge(m2,tmp2-s/nadj,tmp2);
	}

      /* pack up to add to interaction */
      mask=tmp1!=0.0;
      ntrue = sum(mask);
      interaction.val <<= pack(tmp1,mask,ntrue);
      interaction.col <<= pack( pcd, mask, ntrue);
      interaction.row <<= array<int>(ntrue, tmp1.size());

      mask=tmp2!=0.0;
      ntrue = sum(mask);
      interaction.val <<= pack( tmp2, mask, ntrue);
      interaction.row <<= pack( pcd, mask, ntrue);
      interaction.col <<= array<int>( ntrue, tmp2.size() );
    }

  create <<=  array<double>(new_sp.size(),0);
  species <<= pcoord(new_sp.size())*nprocs() + sp_cntr;
  sp_cntr+=new_sp.size()*nprocs();

  assert(sum(mutation<0)==0);
  assert(sum(interaction.diag>=0)==0);

  foodweb = interaction;
}



array<double> PanmicticModel::lifetimes() 
{ 
  array<double> lifetimes; 

  for (size_t i=0; i<species.size(); i++) 
    {
      if (create[i]==0 && density[i]>10) 
	create[i] = tstep;
      else if (create[i]>0 && density[i]==0) 
	/* extinction */
	{
	  lifetimes <<= tstep - create[i];
	  create[i]=0;
	}
    }
  return lifetimes;
}


bool ConnectionPlot::redraw(int x0, int y0, int width, int height)
{
  if (!surface) return false;
  const double scale=4;
  int low_colour, high_colour;
  auto& row=connections.row;
  auto& col=connections.col;
  ecolab::array<int> enum_clusters(connections.diag.size());
  enum_clusters=1; 
  enum_clusters=enumerate(enum_clusters);
  for (unsigned i=0; i<row.size(); i++)
    {
      high_colour = enum_clusters[col[i]];
      low_colour =  enum_clusters[row[i]];
      
      if (high_colour<low_colour) std::swap(high_colour,low_colour);
      enum_clusters = merge( enum_clusters==high_colour, 
			     low_colour, enum_clusters);
    }
      
  /* for grouping species into their ecologies */
  
  ecolab::array<int> map(density.size()), mask;
  map=-1;
  for (int i=0; i<=max(enum_clusters); i++)
    {
      mask = enum_clusters==i;
      map = merge(  mask, enumerate(mask)+max(map)+1, map);
    }

  auto cairo=surface->cairo();
  for (unsigned i=0; i<row.size(); i++)
    {
      cairo_rectangle(cairo, map[row[i]]*scale, map[col[i]]*scale, scale, scale);
      //cairo_stroke_preserve(cairo);
      if (density[row[i]]==0||density[col[i]]==0)
        cairo_set_source_rgb(cairo, 0xf5/256.0, 0xde/256.0, 0xb3/256.0); // wheat
      else
        {
          auto& colour=palette[enum_clusters[row[i]]%paletteSz];
          cairo_set_source_rgb(cairo,colour.r,colour.g,colour.b);
        }
      cairo_fill(cairo);
    }

  // diagonals
  for (size_t i=0; i<density.size(); i++)
    {
      cairo_rectangle(cairo, map[i]*scale, map[i]*scale, scale,scale);
      if (density[i]==0)
        cairo_set_source_rgb(cairo, 0xf5/256.0, 0xde/256.0, 0xb3/256.0); // wheat
      else
        {
          auto& colour=palette[enum_clusters[i]%paletteSz];
          cairo_set_source_rgb(cairo,colour.r,colour.g,colour.b);
        }
      cairo_fill(cairo);
    }
  return true;
}

