/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#define kmp_malloc std::malloc
#define kmp_free std::free
#include <ecolab.h>
#include <analysis.h>

#if defined(__GNUC__) && !defined(__ICC) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

#define MAP vmap
#include "graphcode.h"
#include "TCL_obj_graphcode.h"
#include "graphcode.cd"
#include "TCL_obj_stl.h"

using namespace ecolab;
using namespace std;
#include "arrays.h"
#include "jellyfish.h"
#include "jellyfish.cd"
#include "ecolab_epilogue.h"

#include <algorithm>

using namespace ecolab;

lake_t lake; make_model(lake);

Register<jlist> dummy;

static gaussrand gr; /* default generator for distributions below */
/* global variables - exposed to TCL, but not checkpointed */
affinerand speed_dist(1,0,&gr); register(speed_dist);
double max_speed; register(max_speed);
affinerand angle_dist(1,0,&gr); register(angle_dist);
affinerand radii_dist(1,0,&gr); register(radii_dist);
affinerand vz_dist(1,0,&gr); register(vz_dist); 
urand uni; register(uni);
double change_prob; register(change_prob);
double vert_change_prob; register(vert_change_prob);
double ideal_depth; register(ideal_depth);
double photo_taxis; register(photo_taxis);
bool colourprocs=false; register(colourprocs);

static inline double sqr(double x) {return x*x;}

static inline double sqr(const Vec<3>& x) 
{
  return sqr(x[0])+sqr(x[1])+sqr(x[2]);
}

static inline double dot(Vec<3>& x, Vec<3>& y)
{
  return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];
}

void lake_t::clear_non_local() 
{
  jelly_map.clear_non_local();
}

bool lake_t::in_shadow(int x, int y)
{
  return bdy[x+width*y]==2||bdy[x-1+width*y]==2||bdy[x+width*(y-1)]==2||
    bdy[x+1+width*y]==2||bdy[x+width*(y+1)]==2;
}

/* define the lake boundaries from the supplied pixmap - photo_name is
   the name of a image photo created in the TCL interpreter */

void lake_t::set_lake(TCL_args args)
{
#ifdef TK
  parallel(args);
  if (myid==0)
    {
      Tk_PhotoImageBlock block; 
      Tk_PhotoGetImage(Tk_FindPhoto(interp(),args),&block);
      width=block.width; height=block.height;
      bdy.resize(width*height);
      size_t i,j,p;
      for (i=0,p=0; i<height; i++)
	for (j=0; j<width; j++,p++)
	  { 
	    int p1=i*block.pitch+j*block.pixelSize;
	    bdy[p] = (block.pixelPtr[p1+block.offset[0]]==0) && 
	      (block.pixelPtr[p1+block.offset[1]]==0);
	  }
      for (p=width+1; p<(height-1)*width-1; p++)
	if (bdy[p] && !(bdy[p-1]&&bdy[p+1]&&bdy[p-width]&&bdy[p+width]))
	      edge_indices<<=p;
    }
#ifdef MPI_SUPPORT
    MPIbuf() << *this << bcast(0) >> *this;
#endif
#endif
}

void jelly_map_t::init_map(unsigned nmapx, unsigned  nmapy, unsigned nmapz)
{
  unsigned p;
  mapid_t imapid(nmapx,nmapy,nmapz);

  /* 
     save existing jellyfish (if any) 
     This is used for restart. We assume the checkpoint file has all jellyfish
     stored (as after a gather), and is read in on proc 0 at least.
     We cannot assume processor counts are the same as the previous run.
  */
  jlist jfish;
  if (myid==0)
    for (omap::iterator i=objects.begin(); i!=objects.end(); i++)
      {
        jlist &cell_list=dynamic_cast<jlist&>(**i);
        for (jlist::iterator j=cell_list.begin(); j!=cell_list.end(); j++)
          jfish.add_jelly(*j);
      }
  objects.clear();

#ifdef THREE_D 
  for (unsigned i=0; i<nmapx; i++)
    for (unsigned j=0; j<nmapy; j++)
      for (unsigned k=0; k<nmapz; k++)
	{
	  objref& o=objects[imapid(i,j,k)];
	  o.proc=(i*nprocs)/nmapx;
	  if (o.proc==myid) AddObject<jlist>(o.ID);
	}
  for (unsigned i=0; i<nmapx; i++)
    for (unsigned j=0; j<nmapy; j++)
      for (unsigned k=0; k<nmapz; k++)
	{
	  /* connect up a Moore neighbourhood */
	  objref& o=objects[imapid(i,j,k)];
	  if (o.proc==myid)
	    {
	      o->push_back(o); /* first link is a self link */
	      for (int ii=-1; ii<=1; ii++)
		for (int jj=-1; jj<=1; jj++)
		  for (int kk=-1; kk<=1; kk++)
		    if (ii!=0 || jj!=0 || kk!=0)
		      o->push_back(objects[imapid(i+ii,j+jj,k+kk)]);
	    }
	}
#else
  for (i=0; i<nmapx; i++)
    for (j=0; j<nmapy; j++)
      {
	objref& o=objects[imapid(i,j)];
	o.proc=(i*nprocs)/nmapx;
	if (o.proc==myid) AddObject(jlist(),o.ID);
      }
  for (i=0; i<nmapx; i++)
    for (j=0; j<nmapy; j++)
      {
	/* connect up a Moore neighbourhood */
	objref& o=objects[imapid(i,j)];
	if (o.proc==myid)
	  {
	    o->push_back(o); /* first link is a self link */
	    for (int ii=-1; ii<=1; ii++)
	      for (int jj=-1; jj<=1; jj++)
		if (ii!=0 || jj!=0)
		  o->push_back(objects[imapid(i+ii,j+jj)]);
	  }
      }
#endif

  /* reload existing jellyfish (if any) */
#ifdef MPI_SUPPORT
  MPIbuf_array sendbuf(nprocs);
  tag++;
  for (jlist::iterator j=jfish.begin(); j!=jfish.end(); j++)
    {
      GraphID_t id=(*j)->mapid();
      if (objects[id].proc==myid)
	cell(id).add_jelly(*j);
      else
	sendbuf[objects[id].proc]<<**j; //load buffers for sending to remote proc
    }
  for (unsigned i=0; i<nprocs; i++)
    if (i!=myid) sendbuf[i] << isend(i,tag);
  for (unsigned i=0; i<nprocs-1; i++)
    {
      MPIbuf buf; buf.get(MPI_ANY_SOURCE,tag);
      while (buf.pos()<buf.size())
	{
	  jellyfish j(false);
	  buf>>j;
	  cell(j.mapid()).add_jelly(j);
	}
    }
#else
  for (jlist::iterator j=jfish.begin(); j!=jfish.end(); j++)
	cell((*j)->mapid()).add_jelly(*j);
#endif
}  

void lake_t::init_map(TCL_args args)
{
  parallel(args);
  jelly_map.init_map(nmapx,nmapy,nmapz);
  jelly_map.rebuild_local_list();
}  

void lake_t::gather()
{
#ifdef MPI_SUPPORT
  if (myid==0) parsend("lake.gather");
#endif
  jelly_map.gather();
}
 
//void lake_t::distribute()  /* distribute jellyfish according to partiti



void lake_t::compute_shadow(TCL_args args)
{
  parallel(args);
  /* normal components of Sun's direction */
  args>>sx>>sy>>sz;
  double slope=-sy/sx;
  int shadl=-mangrove_height*(fabs(slope)<1?sx:-sy)/sz;
  int const shadow_val=180;
  if (shadl==0) return;

  //  int i,j;

  if (sz>0)
    {
      /* reconstruct lake image */
      for (size_t i=0; i<width*height; i++) bdy[i]=bdy[i]!=0;
      for (size_t p=0; p<edge_indices.size(); p++)
	  { 
	    div_t d=div(edge_indices[p],width); size_t i=d.rem, j=d.quot;
	      {
		/* draw a shadow line from i,j */
		for (int k=(shadl>0)?0:shadl+1; k<((shadl>0)?shadl:1); k++) 
		  {
		    int x,y;
		    if (fabs(slope)<1)
		      {x=i+k; y=j+slope*k;}
		    else
		      {y=j+k; x=i+k/slope;}
		    if (in_water(x,y)) bdy[x+y*width]=2;
		  }
	      }
	  }
    }
  else /* everything's in shadow */
    for (size_t i=0; i<width*height; i++) bdy[i]=2*(bdy[i]!=0);
 
  shadow_updated=1;
}

/* add_jellyfish <no to add> <speed distribution> */

void lake_t::add_jellyfish(TCL_args args)
{
  parallel(args);
  if (width*height==0) 
    throw error("Cannot add jellyfish to a zero sized lake");
  int njellies=args; 
  jelly_map.gather();
  if (myid==0)
    for (int i=0; i<njellies; i++) 
      {
        int cnt;
        for (cnt=0; cnt<10; cnt++)
          {
            jellyfish j(true);
            jlist& jl=jelly_map.cell(j.mapid());
            // check to see that jellyfish is not overlapping any other jellyfish in neighbourhood
            double mindist=std::numeric_limits<double>::max();
            for (Ptrlist::iterator n=static_cast<GRAPHCODE_NS::object&>(jl).begin(); 
                 n!=static_cast<GRAPHCODE_NS::object&>(jl).end() && mindist>0; n++)
              mindist = std::min(mindist, jelly_map.cell(*n).mindist(j));
            if (mindist>0) 
              {
                jl.add_jelly(j);
                break;
              }
          }
        if (cnt==10) throw error("lake is too full");
      }
  jelly_map.Distribute_Objects();
  jelly_map.clear_non_local();
  jelly_map.Partition_Objects(); 
}

double jlist::mindist(const jellyfish& j)
{
  double mindist=std::numeric_limits<double>::max();
  for (jlist::iterator i=begin(); i!=end(); i++)
    {
      double dsq=sqr(j.pos-(*i)->pos);
      double d=sqrt(dsq) - j.radius - (*i)->radius;
      if (mindist > d)
        mindist=d;
    }
  return mindist;
}

void lake_t::draw(TCL_args args)
{
#ifdef TK
  eco_string canvas=args;
  tclcmd c;

  if (shadow_updated)
    {
      /* draw the shadow */
      static struct shadow
      {
	char *sh_dat;
	int width;
	shadow(int w, int h) {
	  if (w%8) w=(w/8+1)*8;  /* pad to multiple of 8 */
	  width=w/8;
	  sh_dat=new char[w*h/8+1];
	  Tk_DefineBitmap(interp(),Tk_GetUid("shadow"),sh_dat,w,h);
	}
	~shadow() {delete [] sh_dat;}
      } shad(width,height);
      for (size_t i=0; i<height; i++)
	for (size_t j=i*shad.width, k=0; k<width; k+=8,j++)
	  {
	    shad.sh_dat[j]=0;
	    /* ensure we don't try to acces beyond row if width % 8 !=0 */
	    for (int l=(width-k)<8? 9-width+k: 1; l<=8 ; l++) 
	      shad.sh_dat[j]=(shad.sh_dat[j]<<1)|(bdy[i*width+(k+8)-l]==2);
	  }
      c << ".lake.canvas delete shadow\n";
      c << ".lake.canvas create bitmap 0 0 -anchor nw -bitmap shadow"<<
	"-foreground #0000C4 -tags shadow\n";
    }
  c << canvas << "delete jfish\n";
  gather();
  for (omap::iterator i=jelly_map.objects.begin(); i!=jelly_map.objects.end(); i++) 
    {
      jlist& jl=dynamic_cast<jlist&>(**i);
      for (jlist::iterator j=jl.begin(); j!=jl.end(); j++)
	(*j)->draw(canvas,i->proc);
    }
#endif
}

void lake_t::draw_density(TCL_args args)
{
#ifdef TK
  gather();

  vector<unsigned> njellies(width*height);
  unsigned maxj=0;
  for (unsigned x=0; x<width; x++)
    for (unsigned y=0; y<height; y++)
      {
        unsigned i=x+width*y;
        for (unsigned z=0; z<nmapz*nmapy*nmapx; z+=nmapy*nmapx)
          {
            jlist& jl=jelly_map.cell(mapid(x,y)+z);
            for (jlist::iterator j=jl.begin(); j!=jl.end(); j++)
              if (unsigned((*j)->x()+.5)==x && unsigned((*j)->y()+.5)==y)
                njellies[i]++;
          }
        if (njellies[i]>maxj) maxj=njellies[i];
      }

  vector<unsigned char> pp(3*width*height,255);
  Tk_PhotoImageBlock block={pp.data(),int(width),int(height),int(width)*3,3,{0,1,2}};
  float invlogmaxj=1/log(maxj+1);
  for (unsigned x=0; x<width; x++)
    for (unsigned y=0; y<height; y++)
      if (in_water(x,y))
        {
          unsigned i=x+width*y;
          unsigned char *p=&pp[3*i];
          int v=255*log(njellies[i])*invlogmaxj;
          p[0]=v;
          p[1]=0;
          p[2]=255;
        }

#if TK_MAJOR_VERSION<=8 && TK_MINOR_VERSION <5
  Tk_PhotoPutBlock(Tk_FindPhoto(interp(),args),&block,0,0,width,height,
                   TK_PHOTO_COMPOSITE_SET);
#else
  Tk_PhotoPutBlock(interp(),Tk_FindPhoto(interp(),args),&block,0,0,width,height,
                   TK_PHOTO_COMPOSITE_SET);
#endif  
#endif
}

inline bool jellyfish::in_shadow() 
{return lake.in_shadow(pos);}

#ifdef THREE_D

bool lake_t::in_shadow(const Vec<3>& p) 
{
  if (lake.sz==0) return false;
  double p2overSz=p[2]/lake.sz;
  int tx=p[0]-lake.sx*p2overSz, ty=p[1]+lake.sy*p2overSz;
  return !in_water(tx,ty) || in_shadow(tx,ty);
}

inline GraphID_t jellyfish::mapid() {return lake.mapid(x(),y(),z());}
inline GraphID_t jellyfish::mapid_next() {return lake.mapid(x()+vx(),y()+vy(),z()+vz());}

#else

bool lake_t::in_shadow(const Vec<2>& p) 
{return lake.sz>0  && lake.in_shadow(p[0],p[1]);}

inline GraphID_t jellyfish::mapid() {return lake.mapid(x(),y());}
inline GraphID_t jellyfish::mapid_next() {return lake.mapid(x()+vx(),y()+vy());}


#endif


void jellyfish::draw(eco_string& canvas,int proc)
{
  tclcmd c;
  palette_class palette;
  c << canvas << "create line" << x() << y() << x()+vx() << y()+vy();
  if (colourprocs)
    c << "-fill"<<palette[proc];
  else if (colour&2) 
    c <<"-fill green";
  else if (colour&1)
      c <<"-fill red";
  c << "-arrow last -tags jfish\n";
}

void jellyfish::jinit()
{
  static int nextid=0;
  id=nextid++;

#ifdef THREE_D
  pos[2]=ideal_depth;
  vel[2]=0;
#endif
  /* pick a random location in lake until we're in the water */
  for (pos[0]=uni.rand()*lake.width, pos[1]=uni.rand()*lake.height; 
       !lake.in_water(x(),y());
       pos[0]=uni.rand()*lake.width, pos[1]=uni.rand()*lake.height);


  /* pick a unformly random direction, and a speed from speed */
  double sp;
  while ((sp=fabs(speed_dist.rand()))>max_speed);
  double angle=6.2831853*uni.rand(); //[0..2pi]
  vel[0]=sp*cos(angle); 
  vel[1]=sp*sin(angle);
  radius=radii_dist.rand();
  colour=0;
}

void jellyfish::change_dir()
{ 
  const double pi=3.1415926;
  /* new heading chosen randomly from a gaussian distribution, centred
     on the sum of the current velocity / speed_dist.sigma and the
     horizontal projection of the sun's position */

  double mx=vx()/speed_dist.scale;
  double my=vy()/speed_dist.scale; 
  if (!in_shadow()) {
    mx+=photo_taxis*lake.sx;
    my-=photo_taxis*lake.sy;
  }

  double sp, sqrtmxmy=sqrt(mx*mx+my*my);
  if (sqrtmxmy<1E-100)
    vel[0]=vel[1]=0;
  else
    {
      while ((sp=fabs(speed_dist.rand()))>max_speed);
      sp/=sqrtmxmy;
      double rand_angle=pi*angle_dist.rand()/180;
      vel[0]=sp*(mx*cos(rand_angle) - my*sin(rand_angle)); /* cos(A+B) formula */
      vel[1]=sp*(mx*sin(rand_angle) + my*cos(rand_angle)); /* sin(A+B) formula */
    }
  
}

bool jellyfish::collision(GraphID_t cell)
{
    /* get list of jellyfish in cell located at x,y */
  assert(lake.back_map.count(cell)); 

  vector<jellyfish>& jellylist=lake.back_map[cell];
  for (vector<jellyfish>::iterator n=jellylist.begin(); n!=jellylist.end(); n++)
    {
      double r01sq=sqr(radius+n->radius);
      Vec<DIM> posdiff(pos-n->pos);
      double diff_endpt=sqr(posdiff+vel);
      if (diff_endpt <= r01sq)
        {
          /* could end up on top of other jfish if it doesn't move */
          return true;
        }
      /* compute minimum expected distance between jellyfish over
         next timestep, using the formula 
         r^2=(x_0-x_1 + (v_0-v_1) t)^2 =at^2+2bt+c,
         min_t r^2 = c-b^2/a, with
         a = (v_0-v_1)^2; b=(v_0-v_1).(x_0-x_1) and c=(x_0-x_1)^2
      */
      Vec<DIM> veldiff(vel-n->vel);
      double a=sqr(veldiff);
      double b=dot(veldiff, posdiff);
      double c=sqr(posdiff);
      if (c==0) continue; //don't compute self-self collision
      if (c<r01sq)
        cout << "warning, jellyfish overlap "<<c<<"<"<<id << " " <<n->id<<endl;
      
      c-=r01sq;

      if ( b<=0 && sqr(b)-a*c >= sqr(a+b)) /* collision happens */
        { 
          colour|=2;
          return true;
        }
    }
  return false;
}

void jellyfish::update()
{
  bool moved=false;
  /* see if we collide with any jellyfish */
  colour&=~2;
  GraphID_t this_cell=mapid(), next_cell;
  Vec<DIM> newpos(pos+vel);

  bool nocollision=true;
  objref& cell=lake.jelly_map.objects[this_cell];
  if (
      lake.in_water(newpos) && 
      (!lake.in_shadow(newpos) || in_shadow()) 
      )
    {
      for (Ptrlist::iterator n=cell->begin(); nocollision && n!=cell->end(); n++)
        nocollision=!collision(n->ID);
      
      if (nocollision)
        swap(pos,newpos);
    }
  else
    {
      vel=-vel; //back up
      //vel*=-1;
      return;
    }
    
  if (uni.rand()<change_prob) change_dir();
#ifdef THREE_D /* vertical velocity dynamics are independent */
  /* if vz==0; and sunlight too weak, go up, otherwise if too strong go down */
  if (vz()==0)
    {
      if (uni.rand()<vert_change_prob || !nocollision)
        {
          if (!in_shadow())
            {
              if (z()<ideal_depth) vel[2]=fabs(vz_dist.rand());
              else if (z()>ideal_depth) vel[2]=-fabs(vz_dist.rand());
              else vel[2]=vz_dist.rand();
            }
          else  vel[2]=vz_dist.rand();
        }
    }
  else if (uni.rand()>vert_change_prob) vel[2]=0;
#endif
}

void lake_t::update(TCL_args args)
{
  parallel(args);
  jelly_map.Prepare_Neighbours(true);
  
  /*
    Prepare backing map as a copy of the objects map 
    (locally stored and cached copies of objects)
  */
  back_map.clear();
  for (omap::iterator i=jelly_map.objects.begin(); 
       i!=jelly_map.objects.end(); i++)
    if (!i->nullref())
      {
        //ensure entry is created, even if no jellyfish are present
        back_map[i->ID].clear();
        for (jlist::iterator j=dynamic_cast<jlist&>(**i).begin(); 
             j!=dynamic_cast<jlist&>(**i).end(); j++)
          back_map[i->ID].push_back(**j);
      }

#ifdef MPI_SUPPORT
  MPIbuf_array emigrants(nprocs);
#endif

  // update jellyfish
  size_t sz=jelly_map.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for (size_t i=0; i<sz; i++)
     {
      clock_t start=clock();
      jlist& jl=dynamic_cast<jlist&>(*jelly_map[i]);
      for (jlist::iterator ji=jl.begin(); ji!=jl.end(); ji++)
        (*ji)->update();
      jl.etime=clock()-start+1;
     }

  // check whether jellyfish has migrated to a neighbouring cell
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
  for (size_t i=0; i<sz; i++)
     {
      clock_t start=clock();
      jlist new_list;
      jlist& jl=dynamic_cast<jlist&>(*jelly_map[i]);
      for (jlist::iterator ji=jl.begin(); ji!=jl.end(); ji++)
	{
          classdesc::ref<jellyfish> j(*ji);
	  objref& target=jelly_map.objects[j->mapid()];
	  if (target.ID != jelly_map[i].ID) /* jellyfish has migrated to next cell */
	    {
	      if (target.proc == myid)  /* next cell is local */
                dynamic_cast<jlist&>(*target).add_jelly(j);
#ifdef MPI_SUPPORT
	      else   /* store jellyfish to be transferred to remote processor */
#ifdef _OPENMP
#pragma omp critical
#endif
		  emigrants[target.proc] << j;
#endif
	    }
	  else
	    new_list.add_jelly(j);
	}
      swap(new_list,jl);
      jl.etime+=clock()-start+1;
    }	

  // transfer migrants to remote processors
#ifdef MPI_SUPPORT
  jelly_map.tag++;
  for (unsigned p=0; p<nprocs; p++)
    if (p!=myid) 
      {
	emigrants[p].isend(p,jelly_map.tag);
      }
  for (unsigned p=0; p<nprocs-1; p++)
    {
      MPIbuf b; b.get(MPI_ANY_SOURCE,jelly_map.tag);
      while (b.pos()<b.size()) 
	{
          classdesc::ref<jellyfish> j;
	  b >> j;
          if (jelly_map.objects[j->mapid()].proc!=myid)
            cout << "jelly_map.objects[j->mapid()].proc="<<jelly_map.objects[j->mapid()].proc<<" on "<<myid<<endl;
	  jelly_map.cell(j->mapid()).add_jelly(j);
	}
    }
#endif

}


void lake_t::update_map(TCL_args args)
{
  parallel(args);
  jelly_map.Partition_Objects();  
  jelly_map.clear_non_local();
}


/* a binary predicate dist_t(i,j) returing true if jellyfish i is
   closer to (mx,my) than jellyfish j */
struct closer_t
{
  double mx,my;
  closer_t(double x,double y): mx(x), my(y) {}
  double sqr(double x) {return x*x;}
  bool operator()
  (const classdesc::ref<jellyfish>& i, const classdesc::ref<jellyfish>& j) 
  {return sqr(i->x()-mx)+sqr(i->y()-my)<sqr(j->x()-mx)+sqr(j->y()-my);}
};

/* select a jellyfish located at x,y, register it, and return a unique id */
eco_string lake_t::select(TCL_args args)
{
  double x,y; args>>x>>y;
  closer_t closer(x,y);
  
#ifdef THREE_D 
  classdesc::ref<jellyfish> selection, tselection;
  bool selected=false;
  for (unsigned z=0; z<nmapz*nmapy*nmapx; z+=nmapy*nmapx)
    {
      jlist& jellies=jelly_map.cell(mapid(x,y)+z);
      if (jellies.size()==0) continue;
      tselection=*min_element(jellies.begin(),jellies.end(),closer);
      if (!selected || closer(tselection,selection))
	selection=tselection;
      selected=true;
    }
  if (!selected) return eco_string();
#else
  jlist& jellies=jelly_map.cell(mapid(x,y));
  if (jellies.size()==0) return eco_string();
  ref<jellyfish> selection=
    *min_element(jellies.begin(),jellies.end(),closer);
#endif

  selection->colour|=1;  

  static int selno=1;
  eco_strstream s; s|"jfish_sel"|selno++;
  TCL_obj(null_TCL_obj,s.str(),*selection);
  return s.str();
}

/* return a list of jellyfish depths, for all jellyfish located in
   cell column containing coordinate x,y */

ecolab::array<double> lake_t::depthlist(TCL_args args)
{
  double x=args, y=args;
  ecolab::array<double> r;
  for (unsigned z=0; z<nmapz*nmapy*nmapx; z+=nmapy*nmapx)
    {
      jlist& c=jelly_map.cell(mapid(x,y)+z);
      for (jlist::iterator i=c.begin(); i!=c.end(); i++)
#ifdef THREE_D
	r<<=(*i)->z();
#else
        r<<0;
#endif
    }
  return r;
}

int lake_t::cell_max(TCL_args args)
{
  parallel(args);
  int max=0;
  for (jelly_map_t::iterator i=jelly_map.begin(); i!=jelly_map.end(); i++)
    {
      int s=dynamic_cast<jlist&>(**i).size();
      if (s>max) max=s;
    }
#ifdef MPI_SUPPORT
  int lmax=max;
  MPI_Reduce(&lmax,&max,1,MPI_INT,MPI_MAX,0,MPI_COMM_WORLD);
#endif
  return max;
}

//NEWCMD(proc_weights,0)
//{
//#ifdef MPI_SUPPORT
//  PARALLEL;
//  double sum=0;
//  for (jelly_map_t::iterator i=lake.jelly_map.begin(); 
//       i!=lake.jelly_map.end(); i++) sum+=i->pwgt();
//  MPIbuf b; b<<sum;
//  tclreturn r;
//  b.gather(0);
//  if (myid==0)
//    while (b.pos<b.size) {b>>sum; r<<sum;}
//#endif
//}

/* incidental support for date calculation */

static int dimoffs[]={31,59,90,120,151,181,212,243,273,304,334,365};
static const char* mon[]=
{"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

NEWCMD(date,1)
{
  int m,d=atoi(argv[1]);
  if (d<=0 || d>365) error("Invalid date");
  for (m=0; d>dimoffs[m]; m++);
  tclreturn() << (m?d-dimoffs[m-1]:d) << mon[m];
}

NEWCMD(month,1)
{tclreturn() << mon[atoi(argv[1])];}

NEWCMD(dayofyear,2)
{
  int m=atoi(argv[2]);
  tclreturn() << (m?dimoffs[m-1]:0) + atoi(argv[1]);
}    

NEWCMD(mapid,2)
{
  tclreturn() << lake.mapid(atof(argv[1]),atof(argv[2]));
}
