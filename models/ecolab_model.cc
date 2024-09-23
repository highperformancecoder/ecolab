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

#include "ecolab_model.cd"

#include "ecolab_epilogue.h"

using namespace classdesc;

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

unsigned EcolabPoint::nsp() const
{return sum(density!=0);}

array<unsigned> SpatialModel::nsp() const
{
  array<unsigned> nsp;
  for (auto& i: objects) nsp<<=i->nsp();
  return nsp;
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

void SpatialModel::condense()
{
  array<int> total_density(species.size());
  for (auto& i: *this) total_density+=i->as<EcolabCell>()->density;
#ifdef MPI_SUPPORT
  array<int> recv(total_density.size());
  MPI_Allreduce(total_density.data(),recv.data(),total_density.size(),MPI_INT,MPI_SUM,MPI_COMM_WORLD);
  total_density.swap(recv);
#endif
  auto mask=total_density != 0;
  
  size_t mask_true=sum(mask);
  if (mask.size()==mask_true) return; /* no change ! */
  ModelData::condense(mask,mask_true);
  for (auto& i: *this) i->as<EcolabCell>()->condense(mask, mask_true);
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

void SpatialModel::mutate()
{
  array<double> mut_scale(sp_sep * repro_rate * mutation * (tstep - last_mut_tstep));
  last_mut_tstep=tstep;
  array<unsigned> new_sp, cell_ids;
  array<unsigned> num_new_sp;
  for (auto& i: *this)
    {
      new_sp <<= i->as<EcolabCell>()->mutate(mut_scale);
      cell_ids <<= array<int>(new_sp.size()-cell_ids.size(),i.id());
    }
//  unsigned offset=species.size(), offi=0;
//  // assign 1 for all new species created in this cell, 0 for the others
//  for (auto& i: *this)
//    {
//      auto& density=i->as<EcolabCell>()->density;
//      density<<=array<int>(new_sp.size(),0);
//      for (size_t j=0; j<num_new_sp[offi]; ++j)
//        {
//          density[j+offset]=1;
//        }
//      offset+=num_new_sp[offi++];
//    }
#ifdef MPI_SUPPORT
  MPIbuf b; b<<new_sp<<cell_ids; b.gather(0);
  if (myid()==0)
    {
      new_sp.resize(0); cell_ids.resize(0);
      for (unsigned i=0; i<nprocs(); i++) 
	{
	  array<int> n,c;
	  b>>n>>c;
	  new_sp<<=n; cell_ids<<=c;
	}
      ModelData::mutate(new_sp);
    }
  MPIbuf() << cell_ids << *(ModelData*)this << bcast(0)
           >> cell_ids >> *(ModelData*)this;
#else
  ModelData::mutate(new_sp);
#endif
  // set the new species density to 1 for those created on this cell
  for (auto& i: *this)
    i->as<EcolabCell>()->density <<= cell_ids==i.id();
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

void SpatialModel::setGrid(size_t nx, size_t ny)
{
  numX=nx; numY=ny;
  for (size_t i=0; i<numX; ++i)
    for (size_t j=0; j<numY; ++j)
      {
        auto o=insertObject(makeId(i,j));
        o.proc(o.id() % nprocs()); // TODO can we get this to work without this.
        // wire up von Neumann neighborhood
        o->neighbours.push_back(makeId(i-1,j));
        o->neighbours.push_back(makeId(i+1,j)); 
        o->neighbours.push_back(makeId(i,j-1)); 
        o->neighbours.push_back(makeId(i,j+1)); 
      }
  //distributeObjects();
  partitionObjects();
}

void SpatialModel::generate(unsigned niter)
{
  if (tstep==0) makeConsistent();
  for (auto& i: *this) i->as<EcolabCell>()->generate(niter,*this);
  tstep+=niter;
}

void SpatialModel::migrate()
{
  /* each cell gets a distinct random salt value */
  // TODO why doesn't this loop work?
//  for (auto& i: *this)
//    (*this)[i]->as<EcolabCell>()->salt=array_urand.rand();

  for (auto& o: objects) o->salt=array_urand.rand();
  
  prepareNeighbours();
  vector<array<int> > delta(size(), array<int>(species.size(),0));

  for (size_t i=0; i<size(); i++)
    { 
      auto& cell=*(*this)[i]->as<EcolabCell>();
      /* loop over neighbours */ 
      for (auto& n: *(*this)[i]) 
	{
          auto& nbr=*n->as<EcolabCell>();
	  array<double> m( double(tstep-last_mig_tstep) * migration * 
                           (nbr.density - cell.density) );
          double salt=(*this)[i].id()<n.id()? cell.salt: nbr.salt;
          delta[i] += array<int>(m + array<double>(m!=0.0)*(2*(m>0.0)-1)) * salt;
	}
    }
  last_mig_tstep=tstep;
  for (size_t i=0; i<size(); i++)
    (*this)[i]->as<EcolabCell>()->density+=delta[i];

  /* assertion testing that population numbers are conserved */
#ifndef NDEBUG
  array<int> ssum(species.size()), s(species.size()); 
  unsigned mig=0, i;
  for (ssum=0, i=0; i<size(); i++)
    {
      ssum+=delta[i];
      for (size_t j=0; j<delta[i].size(); j++)
	mig+=abs(delta[i][j]);
    }
#ifdef MPI_SUPPORT
  MPI_Reduce(ssum.data(),s.data(),s.size(),MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
  ssum=s;
  int m;
  MPI_Reduce(&mig,&m,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
  mig=m;
#endif
  if (myid==0) assert(sum(ssum==0)==int(ssum.size()));
#endif

}


void SpatialModel::makeConsistent()
{
  // all cells must have same number of species. Pack out with zero density if necessary
  unsigned long nsp=0;
  for (auto& i: *this) nsp=max(nsp,i->as<EcolabCell>()->density.size());
#ifdef MPI_SUPPORT
  MPI_Allreduce(MPI_IN_PLACE,&nsp,1,MPI_UNSIGNED_LONG,MPI_MAX,MPI_COMM_WORLD);
#endif
  for (auto& i: *this)
    i->as<EcolabCell>()->density<<=array<int>(nsp-i->as<EcolabCell>()->density.size(),0);
  ModelData::makeConsistent(nsp);
}
