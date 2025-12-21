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
}


namespace model
{
  PanmicticModel panmictic_ecolab;
  CLASSDESC_ADD_GLOBAL(panmictic_ecolab);
  DeviceType<SpatialModel> spatial_ecolab;
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

/* Rounding function, randomly round up or down, in the range 0..INT_MAX */
template <class B>
int EcolabPoint<B>::ROUND(Float x) 
{
  Float dum;
  const Float maxInt=Float(std::numeric_limits<int>::max()-1);
  if (x<0) x=0;
  if (x>maxInt) x=maxInt;
  //syclPrintf("ROUND inner x=%g, modf=%g, rand()=%g, rand.max=%g, rand.min=%g\n",x,std::fabs(std::modf(x,&dum)),rand(),rand.max(),rand.min());
  return std::fabs(std::modf(x,&dum))*(rand.max()-rand.min()) > (rand()-rand.min()) ?
    (int)x+1 : (int)x;
}

template <class E, class P>
struct RoundArray
{
  const E& expr;
  P& point;
  RoundArray(P& point, const E& expr): expr(expr), point(point) {}
  using value_type=int;
  size_t size() const {return expr.size();}
  int operator[](size_t i) const //{return point.ROUND(expr[i]);}
  {
    auto r=point.ROUND(expr[i]);
    //syclPrintf("ROUND: %d, %g=%d\n",i,expr[i],r);
    return r;
  }
};

namespace ecolab::array_ns
{template <class E, class P> struct is_expression<RoundArray<E,P>>: public true_type {};}

template <class B>
template <class E>
RoundArray<E,EcolabPoint<B>> EcolabPoint<B>::roundArray(const E& expr)
{return RoundArray<E,EcolabPoint<B>>(*this,expr);}

template <> void setArray(array<int,std::allocator<int>>& x, const array<int>& y)
{x=y;}
template <> array<int> getArray(const array<int,std::allocator<int>>& x) {return x;}

#ifdef SYCL_LANGUAGE_VERSION
template <>
array<int> getArray(const array<int,ecolab::CellBase::CellAllocator<int>>& x) 
{
  DeviceType<size_t> size;
  DeviceType<const int*> xData;
  syclQ().single_task([size=&*size,xData=&*xData,x=&x](){
    *size=x->size();
    *xData=x->data();
  }).wait();
  array<int> r(*size);
  syclQ().copy(*xData,r.data(),*size);
  return r;
}

template <>
void setArray(array<int,ecolab::CellBase::CellAllocator<int>>& x, const array<int>& y)
{
  auto size=y.size();
  DeviceType<int*> xData;
  syclQ().single_task([size,x=&x,xData=&*xData](){
    //array<int,typename B::template CellAllocator<int>> tmp(size,this->template allocator<int>());
    //m_density.swap(tmp);
    x->resize(size);
    *xData=x->data(); //return allocated data pointer to host
  }).wait();
  syclQ().copy(y.data(),*xData,size);
}
#endif


template <class B>
void EcolabPoint<B>::generate(unsigned niter, const ModelData& model)
{
#ifdef __SYCL_DEVICE_ONLY__
  Float* interactionResult=groupBuffer<Float,SpatialModel::log2MaxNsp>(density.size());
#else
  array<Float> interactionResult(density.size());
#endif
  for (unsigned step=0; step<niter; step++)
    {
      array_ns::map(density.size(),  [&](size_t i){
        interactionResult[i]=model.interaction.diag[i]*density[i];
      });
      groupBarrier();
      array_ns::map(model.interaction.row.size(), [&](size_t i){
#ifdef __SYCL_DEVICE_ONLY__
        sycl::atomic_ref<Float, sycl::memory_order::relaxed, sycl::memory_scope::work_group>
#endif
          (interactionResult[model.interaction.row[i]]) +=
          model.interaction.val[i]*density[model.interaction.col[i]];
      });
      
      groupBarrier();
      array_ns::map(density.size(),  [&](size_t i){
        density[i]=ROUND(density[i] + density[i] * (model.repro_rate[i] + interactionResult[i]));
      });
      groupBarrier();
    }
//  // sequential/non-GPU version
//  for (unsigned i=0; i<niter; i++)
//    {density = roundArray(density + density * (model.repro_rate + model.interaction*density));}
}

template <class B>
unsigned EcolabPoint<B>::nsp() const
{return sum(density!=0);}

array<unsigned> SpatialModel::nsp() const
{
  array<unsigned> nsp;
  for (auto& i: objects) nsp<<=i->nsp();
  return nsp;
}

template <class B>
void EcolabPoint<B>::condense(const array<bool>& mask, size_t mask_true)
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

unsigned PanmicticModel::condense()
{
  auto mask=density != 0;
  size_t mask_true=sum(mask);
  if (mask.size()==mask_true) return 0; /* no change ! */
  ModelData::condense(mask,mask_true);
  EcolabPoint::condense(mask, mask_true);
  return mask.size()-mask_true;
}

unsigned SpatialModel::condense()
{
  array<int> total_density(species.size());
  for (auto& i: *this) total_density+=i->as<EcolabCell>()->density; // TODO
#ifdef MPI_SUPPORT
  array<int> recv(total_density.size());
  MPI_Allreduce(total_density.data(),recv.data(),total_density.size(),MPI_INT,MPI_SUM,MPI_COMM_WORLD);
  total_density.swap(recv);
#endif
  auto mask=total_density != 0;
  
  size_t mask_true=sum(mask);
  if (mask.size()==mask_true) return 0; /* no change ! */
  ModelData::condense(mask,mask_true);
  for (auto& i: *this) i->as<EcolabCell>()->condense(mask, mask_true);
  return mask.size()-mask_true;
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

  
  
  DeviceType<array<Float,graphcode::Allocator<Float>>> mut_scale;
#ifdef SYCL_LANGUAGE_VERSION
  using ArrayAlloc=CellBase::CellAllocator<unsigned>;
  using NewSpAlloc=graphcode::Allocator<array<unsigned,ArrayAlloc>>;
  vector<array<unsigned,ArrayAlloc>,NewSpAlloc> newSp
    (size(),NewSpAlloc(syclQ(),sycl::usm::alloc::shared));
  mut_scale->allocator(graphcode::Allocator<Float>(syclQ(),sycl::usm::alloc::shared));
#else
  vector<array<unsigned>> newSp(size());
#endif
  *mut_scale=sp_sep * repro_rate * mutation * int(tstep - last_mut_tstep);
  last_mut_tstep=tstep;

  groupedForAll([newSp=newSp.data(),mut_scale=&*mut_scale,this](EcolabCell& c) {
    auto tmp=c.mutate(*mut_scale);
#ifdef __SYCL_DEVICE_ONLY__
    if (syclGroup().get_local_linear_id()==0 && tmp.size())
#endif
      newSp[c.idx()]=tmp;
  });

  array<unsigned> new_sp;
  //DeviceType<array<unsigned,graphcode::Allocator<unsigned>>> cell_ids;
  auto cell_ids=make_unique<array<unsigned>>();
#ifdef SYCL_LANGUAGE_VERSION
//  cell_ids->allocator(graphcode::Allocator<unsigned>(syclQ(),sycl::usm::alloc::shared));
  syncThreads();
#endif


  
  // TODO - this is a kind of scan - can it be done on device?
  size_t j=0;
  for (auto& i: *this)
    {
      new_sp<<=newSp[j];
      (*cell_ids)<<= array<unsigned>(new_sp.size()-cell_ids->size(),i.id());
    }
  
#ifdef MPI_SUPPORT
  MPIbuf b; b<<new_sp<<(*cell_ids); b.gather(0);
  if (myid()==0)
    {
      new_sp.resize(0); cell_ids->resize(0);
      for (unsigned i=0; i<nprocs(); i++) 
	{
	  array<int> n,c;
	  b>>n>>c;
	  new_sp<<=n; (*cell_ids)<<=c;
	}
      ModelData::mutate(new_sp);
    }
  MPIbuf() << (*cell_ids) << *(ModelData*)this << bcast(0)
           >> (*cell_ids) >> *(ModelData*)this;
#else
  ModelData::mutate(new_sp);
#endif
  if (new_sp.size()==0) return;
  // set the new species density to 1 for those created on this cell
  hostForAll([=,cell_ids=&*cell_ids,this](EcolabCell& c) {
    //for (auto& i: *this) {
    //auto& c=*i->as<Cell>();
    c.density <<= (*cell_ids)==c.id;
    //}
  });
}

template <class B>
template <class E>
array<unsigned,typename EcolabPoint<B>::template Allocator<unsigned>>
EcolabPoint<B>::mutate(const E& mut_scale)
{
  /* calculate the number of mutants each species produces */
#ifdef __SYCL_DEVICE_ONLY__
  auto nsp=density.size();
  auto speciations=groupBuffer<unsigned,SpatialModel::log2MaxNsp>(nsp);
  groupBarrier();
  asg_v(speciations, nsp, roundArray(mut_scale * density));

  auto numNewSp=groupBuffer<unsigned,SpatialModel::log2MaxNsp>(syclGroup().get_group_linear_range());
  numNewSp[syclGroup().get_local_linear_id()]=0;

  auto myId=syclGroup().get_local_linear_id();
  
  array_ns::map(nsp, [&](size_t i) {
    numNewSp[myId]+=speciations[i];
  });

  auto offsets=groupBuffer<unsigned,SpatialModel::log2MaxNsp>(syclGroup().get_group_linear_range()+1);
  if (syclGroup().leader())
    {
      offsets[0]=0;
      for (size_t i=1; i<=syclGroup().get_group_linear_range(); ++i)
        offsets[i]=offsets[i-1]+numNewSp[i-1];
    }
  groupBarrier();
  if (offsets[syclGroup().get_group_linear_range()])
    array_ns::asg_minus_v(density.data(), nsp, speciations);
  GroupLocal<array<unsigned,typename EcolabPoint<B>::template Allocator<unsigned>>> new_sp
    (offsets[syclGroup().get_group_linear_range()], this->template allocator<unsigned>());
  groupBarrier();


  // gen_index
  auto p=offsets[myId];
  auto n=new_sp->data();
  array_ns::map(nsp, [&](size_t i) {
    for (size_t j=0; j<speciations[i]; ++j) {
      n[p++]=i;
    }
  });

  groupBarrier();
  if (syclGroup().get_local_linear_id()==0)
    return *new_sp;
  else
    return {};
#else
  array<unsigned> speciations=roundArray(mut_scale * density);
  auto new_sp = gen_index(speciations);
  if (new_sp.size())
    density-=speciations;
  return new_sp;
#endif
}

// Given by the variance of the offdiagonal components, when each diagonal component is normalised to 1
double ModelData::connectivity() const
{
  if (species.size()==0) return 0;
  array<double> v=interaction.val/sqrt(interaction.diag[interaction.row]*interaction.diag[interaction.col]);
  // divide by total number of offdiagonal components, as the data is sparse
  return sum(v*v)/(species.size()*(species.size()-1));
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
	pos = (int)((tmp.size()-1) * ((double) rand()/RAND_MAX) +.5);
	if (tmp[pos]==0.0)
	  tmp[pos] = range *((double) rand()/RAND_MAX) + minval;
      }
  else
    for (unsigned j=0; j<ntrue; j++)
      {
	pos = (int)((tmp.size()-1) * ((double) rand()/RAND_MAX) +.5);
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
  objects.clear();
  for (size_t i=0; i<numX; ++i)
    for (size_t j=0; j<numY; ++j)
      {
        auto o=insertObject(makeId(i,j));
        auto& c=*o->as<EcolabCell>();
        c.id=o.id();
        c.rand.seed(o.id());
        o.proc(o.id() % nprocs()); // TODO can we get this to work without this.
        // wire up von Neumann neighborhood
        if (i>0)
          o->neighbours.push_back(makeId(i-1,j));
        if (i<numX-1)
          o->neighbours.push_back(makeId(i+1,j));
        if (j>0)
          o->neighbours.push_back(makeId(i,j-1));
        if (j<numY-1)
          o->neighbours.push_back(makeId(i,j+1)); 
      }
  rebuildPtrLists();
  for (auto& i: objects)
    maxNbrs=std::max(maxNbrs, i->neighbours.size());
}

void SpatialModel::generate(unsigned niter)
{
  if (tstep==0) makeConsistent();
  groupedForAll([=,this](EcolabCell& c) {
    c.generate(niter,*this);
  });
  tstep+=niter;
}

unsigned SpatialModel::migrate()
{
  /* each cell gets a distinct random salt value */
  hostForAll([=,this](EcolabCell& c) {c.salt=Float(c.rand()-c.rand.min())/(c.rand.max()-c.rand.min());});
  
  prepareNeighbours();

  vector<array<Float>> delta(size(), array<Float>(species.size(),0));
  
  hostForAll([&,this](EcolabCell& c) {
    auto mm=(tstep-last_mig_tstep) * migration;
    // migration is capped by actual population levels
    const Float cap=1.0/maxNbrs;
    array<Float> capped_migration = merge(mm>cap,cap,mm);
    /* loop over neighbours */
    for (auto& n: c) 
      {
        auto& nbr=*n->as<EcolabCell>();
        Float salt=c.idx()<nbr.idx()? c.salt: nbr.salt;
        array<Float> m=capped_migration * (nbr.density-c.density);
        delta[c.idx()]+=m*(1 + salt * (abs(m)<cap));
        assert(all(c.density>=-delta[c.idx()]));
      }
  });

  array<int> ssum(species.size(),0);
  size_t totalMigration=0;

  vector<size_t> negativeDensityIdx;
  size_t numCells=size();
#ifdef _OPENMP
#pragma omp parallel for reduction(+:totalMigration)
#endif
  for (size_t i=0; i<numCells; ++i)
    {
      auto& c=*(*this)[i]->as<EcolabCell>();
      c.density+=delta[c.idx()];
      assert(all(c.density>=0));
      totalMigration+=sum(abs(delta[c.idx()]));
      //      if (sum(c.density<0))
      //#ifdef _OPENMP
      //#pragma omp critical
      //#endif
      //       negativeDensityIdx.push_back(c.idx());
#if !defined(NDEBUG)
#ifdef _OPENMP
#pragma omp critical
#endif
      ssum+=delta[c.idx()];
#endif
    }
  last_mig_tstep=tstep;

//  // if any density values are -ve, then adjust migration from neighbours.
//  // loop run sequentially to resolve race condition
//  for (auto i: negativeDensityIdx)
//    {
//      auto& c=*(*this)[i]->as<EcolabCell>();
//      for (auto j=0; j<c.density.size(); ++j)
//        while (c.density[j]<0)
//          {
//            assert(c.size()>0);
//            int adjust=-(c.density[j]+c.size()-1)/c.size();
//            for (auto& n: c)
//              {
//                auto& nbr=*n->as<EcolabCell>();
//                auto nbrAdjust=std::min(nbr.density[j], adjust);
//                c.density[j]+=nbrAdjust;
//                nbr.density[j]-=nbrAdjust;
//              }
//          }
//    }
  
#ifdef MPI_SUPPORT
  if (myid()==0)
    MPI_Reduce(MPI_IN_PLACE, &totalMigration,1,MPI_UNSIGNED,MPI_SUM,0,MPI_COMM_WORLD);
  else
    MPI_Reduce(&totalMigration,nullptr,1,MPI_UNSIGNED,MPI_SUM,0,MPI_COMM_WORLD);
#endif
  
  /* assertion testing that population numbers are conserved */
#if !defined(NDEBUG) && !defined(SYCL_LANGUAGE_VERSION)
#ifdef MPI_SUPPORT
  array<int> s(species.size()); 
  MPI_Reduce(ssum.data(),s.data(),s.size(),MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
  ssum=s;
#endif
  if (sum(ssum==0)!=int(ssum.size()))
    {
      for (size_t i=0; i<ssum.size(); ++i)
        if (ssum[i])
          {
            cout<<"species "<<i<<":"<<endl;
            for (size_t idx=0; idx<size(); ++idx)
              if (delta[idx][i])
                {
                  auto& c=*(*this)[idx]->as<EcolabCell>();
                  cout<<"  delta "<<c.id<<"="<<delta[idx][i]<<" n="<<c.density[i]<<endl;
                  for (auto& n: c)
                    {
                      auto& nbr=*n->as<EcolabCell>();
                      cout<<"    nbr:"<<nbr.id<<"="<<nbr.density[i]<<endl;
                    }
                }                    
          }
    }
  if (myid()==0) assert(sum(ssum==0)==int(ssum.size()));
#endif
  return totalMigration/2;
}

void ModelData::makeConsistent(size_t nsp)
{
#ifdef SYCL_LANGUAGE_VERSION
  if (sycl::get_pointer_type(species.data(),syclQ().get_context())!=sycl::usm::alloc::shared)
    {
      FAlloc falloc(syclQ(),sycl::usm::alloc::shared);
      species.allocator(graphcode::Allocator<int>(syclQ(),sycl::usm::alloc::shared));
      create.allocator(falloc);
      repro_rate.allocator(falloc);
      mutation.allocator(falloc);
      migration.allocator(falloc);
      interaction.setAllocators
        (graphcode::Allocator<unsigned>(syclQ(),sycl::usm::alloc::shared),falloc);
    }
#endif
  if (!species.size())
    {
      species=pcoord(nsp);
    }
  
  if (!create.size()) create.resize(species.size(),0);
  if (!mutation.size()) mutation.resize(species.size(),0);
  if (!migration.size()) migration.resize(species.size(),0);
}

void SpatialModel::setDensitiesShared()
{
#ifdef SYCL_LANGUAGE_VERSION
  groupedForAll([=,this](EcolabCell& c) {
    c.memAlloc=sharedMemAlloc;
    c.density.allocator(c.allocator<int>());
  });
#endif
}
    
void SpatialModel::setDensitiesDevice()
{
#ifdef SYCL_LANGUAGE_VERSION
  groupedForAll([=,this](EcolabCell& c) {
    c.memAlloc=deviceMemAlloc;
    c.density.allocator(c.allocator<int>());
  });
#endif
}

void SpatialModel::makeConsistent()
{
  // all cells must have same number of species. Pack out with zero density if necessary
  size_t nsp=species.size();
  nsp=max(nsp, [](const EcolabCell& c) {return c.density.size();});
#ifdef MPI_SUPPORT
  MPI_Allreduce(MPI_IN_PLACE,&nsp,1,MPI_UNSIGNED_LONG,MPI_MAX,MPI_COMM_WORLD);
#endif
  hostForAll([=,this](EcolabCell& c) {
#ifdef SYCL_LANGUAGE_VERSION
        if (!c.memAlloc) c.memAlloc=sharedMemAlloc;
#endif
        // not needed, as we're not resizing density on device
        //if (nsp>c.density.size()) c.density.allocator(c.allocator<int>());
  });
  ModelData::makeConsistent(nsp);
  syncThreads();
}
