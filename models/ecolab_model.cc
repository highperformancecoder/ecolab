/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "graphcode.h"
#include "arrays.h"
using namespace ecolab;
using array_ns::array;
using array_ns::pack;
using array_ns::pcoord;
#include "sparse_mat.h" 

#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#include "pythonBuffer.h"
#include "ecolab_model.h"
#include "plot.h"
#include "ecolab_model.cd"
#include "graphcode.cd"
#include "object.cd"
#include "poly.cd"
#include "ecolab_epilogue.h"

namespace classdesc
{
  template <> inline std::string typeName< ::GRAPHCODE_NS::hmap >()
  {return "::GRAPHCODE_NS::hmap";}
}

namespace classdesc_access
{
  template <>
  struct access_RESTProcess<::GRAPHCODE_NS::hmap>:
    public classdesc::NullDescriptor<RESTProcess_t> {};
}

namespace model
{
  ecolab_model ecolab;
  CLASSDESC_ADD_GLOBAL(ecolab);
  CLASSDESC_DECLARE_TYPE(Plot);
  CLASSDESC_PYTHON_MODULE(ecolab_model);
}

void ecolab_grid::set_grid(unsigned x, unsigned y)
{
  //parallel(args);
  if (x*y>1)
    {
      Grid2D::clear();
      objects.clear();
      instantiate(x,y);
      uni.Seed(myid()+1); /*ensure uni has distinct random seed on each processor*/
    }
}

string ecolab_grid::get(unsigned x, unsigned y)
{
  //  std::string cmd((char*)args[-1]);
  eco_strstream name;
//  std::string::size_type dotpos=cmd.rfind('.');
//  if (dotpos!=std::string::npos)
//    name|cmd.substr(0,dotpos);
//  else
//    name|cmd;
//  unsigned x=args, y=args;
//  name|'('|x|','|y|')';
//  ::TCL_obj(null_TCL_obj,name.str(),cell(objects[x+y*xsize]));
  return name.str(); 
}

//void ecolab_grid::forall(TCL_args args)
//{
//  typedef void (ecolab_point::*epm_ptr)(TCL_args);
//  declare(method, epm_ptr, (char*)args);
//  for (iterator i=begin(); i!=end(); i++) (cell(*i).*method)(args);
//}

void ecolab_model::distribute_cells()
{
  //parallel(args);
#ifdef MPI_SUPPORT
  MPIbuf() << *this << bcast(0) >> *this;
  rebuild_local_list();
#endif
}

void ecolab_grid::gather()
{
#ifdef MPI_SUPPORT
  if (myid()==0) parsend("ecolab.gather");
#endif
  Grid2D::gather();
}

void ecolab_model::random_interaction(unsigned conn, double sigma)
{
  interaction.init_rand(conn,sigma);  
  foodweb = interaction;
}

void ecolab_model::generate(unsigned niter)
{
  //parallel(args);
  if (tstep==0) make_consistent();
//  for (iterator i=begin(); i!=end(); i++) 
//    cell(*i).generate(niter);
  generate_point(niter);
  tstep+=niter;
} 

void ecolab_point_data::generate_point(unsigned niter)
{ 
  array<double> n(density);
  for (unsigned i=0; i<niter; i++)
    {n += (model::ecolab.repro_rate + model::ecolab.interaction*n) * n ;}
  density = ROUND(n);
}


void ecolab_point_data::condense_point(const array<bool>& mask, unsigned mask_true)
{
  density = pack( density, mask, mask_true); 
}

void ecolab_model::condense()   /* remove extinct species */
{
  //parallel(args);
  array<int> lmask(species.size()), map /* extinctions*/;
  array<bool> mask, mask_off;
  unsigned mask_true;
 
  lmask=0;
  for (iterator i=begin(); i!=end(); i++)
    lmask |= cell(*i).density != 0;
#ifdef MPI_SUPPORT
  array<int> lmask1(lmask.size());
  MPI_Reduce(lmask.data(),lmask1.data(),lmask.size(),MPI_INT,  MPI_LOR,
	     0,MPI_COMM_WORLD);
  MPI_Bcast(lmask1.data(),lmask1.size(),MPI_INT,0,MPI_COMM_WORLD);
  mask=lmask1;
#else
  mask=lmask;
#endif
  mask_true=sum(mask);
  if (species.size()==mask_true) return; /* no change ! */

  map = enumerate( mask );
  mask_off = mask[ interaction.row ] && mask[ interaction.col ];

  // bugger you g++!
  model_data::create = pack(model_data::create, mask, mask_true); 
  species = pack(species, mask, mask_true); 
  repro_rate = pack(repro_rate, mask, mask_true); 
  mutation = pack(mutation, mask, mask_true); 
  migration = pack(migration, mask, mask_true);
  interaction.diag = pack(interaction.diag, mask, mask_true);

  for (iterator i=begin(); i!=end(); i++) cell(*i).condense_point( mask, mask_true);

  mask_true=sum(mask_off);

  interaction.val = pack(interaction.val, mask_off, mask_true); 
  interaction.row = map[pack(interaction.row, mask_off,mask_true)];
  interaction.col = map[pack(interaction.col, mask_off,mask_true)];

  foodweb = interaction;
}

/* 
   Mutate operator 
*/


void ecolab_model::mutate()
{
  //parallel(args);
  array<double> mut_scale(sp_sep * repro_rate * mutation * (tstep - last_mut_tstep));
  last_mut_tstep=tstep;
  array<unsigned> new_sp, cell_ids;
  for (iterator c=begin(); c!=end(); c++) 
    {
      new_sp <<= cell(*c).mutate_point(mut_scale);
      cell_ids <<= array<int>(new_sp.size()-cell_ids.size(),c->ID);
    }
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
      mutate_model(new_sp);
    }
  MPIbuf() << cell_ids << *(model_data*)this << bcast(0)
	   >> cell_ids >> *(model_data*)this;
#else
  mutate_model(new_sp);
#endif
  for (iterator c=begin(); c!=end(); c++) 
    cell(*c).density <<= cell_ids == c->ID;
}

array<int> ecolab_point_data::mutate_point(const array<double>& mut_scale)
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
void ecolab_model::mutate_model(array<int> new_sp)
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

  model_data::create <<=  array<double>(new_sp.size(),0);
  species <<= pcoord(new_sp.size())*nprocs() + sp_cntr;
  sp_cntr+=new_sp.size()*nprocs();

  assert(sum(mutation<0)==0);
  assert(sum(interaction.diag>=0)==0);

  foodweb = interaction;
}



array<double> ecolab_model::lifetimes() 
{ 
  gather();
  array<double> lifetimes; 

  for (size_t i=0; i<species.size(); i++) 
    {
      unsigned d=0;
      for (omap::iterator c=objects.begin(); c!=objects.end(); c++)
	d+=cell(*c).density[i];
      if (model_data::create[i]==0 && d>10) 
	model_data::create[i] = tstep;
      else if (model_data::create[i]>0 && d==0) 
	/* extinction */
	{
	  lifetimes <<= tstep - model_data::create[i];
	  model_data::create[i]=0;
	}
    }
  return lifetimes;
}

double which_salt(const ecolab_point& x, 
		  const ecolab_point& y, unsigned npins)
{
  if (x[0].ID>y[0].ID)
    if (x[0].ID-y[0].ID > 0.5*npins) return y.salt;
    else return x.salt;
  else
    if (y[0].ID-x[0].ID > 0.5*npins) return x.salt;
    else return y.salt;
}    

void ecolab_model::migrate()
{
  //parallel(args);
  /* refer to the local cell list */
  Ptrlist& cells=*static_cast<Graph*>(this); 

  /* each cell gets a distinct random salt value */
  for (iterator i=begin(); i!=end(); i++)
    cell(*i).salt=uni.rand();
  Prepare_Neighbours();

  vector<array<int> > delta(cells.size(), array<int>(species.size(),0));

  for (size_t i=0; i<cells.size(); i++)
    { 
      ecolab_point& cell=::cell(cells[i]);
      Ptrlist& nbrs=cell;
      unsigned nnbrs=nbrs.size();
      /* loop over neighbours */ 
      for (unsigned j=0; j<nnbrs; j++) 
	{
          ecolab_point& nbr=::cell(nbrs[j]);
	  array<double> m( double(tstep-last_mig_tstep) * migration * 
	    (nbr.density - cell.density) );
	  //	  cout << m << endl;
	  delta[i] += (array<int>) (m + (array<double>)((m!=0.0)*(2*(m>0.0)-1)) * 
			      which_salt(cell, nbr, objects.size()) );
	}
    }
  last_mig_tstep=tstep;

  for (size_t i=0; i<cells.size(); i++)
    cell(cells[i]).density+=delta[i];
  
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
  if (myid()==0) assert(sum(ssum==0)==int(ssum.size()));
#endif
}

