/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <vector>
#include <classdesc_access.h>
#include <pack_stl.h>
#include <time.h>

using graphcode_vmap::Graph;
using graphcode_vmap::GraphID_t;
using graphcode_vmap::bad_ID;
using graphcode_vmap::Wrap;
typedef graphcode_vmap::object object;
using graphcode_vmap::objref;
using graphcode_vmap::omap;
using graphcode_vmap::Ptrlist;

using namespace ecolab;
using ecolab::array_ns::array;

#ifdef THREE_D
const int DIM=3;
#else
const int DIM=2;
#endif

template <int D>
class Vec
{
public: //should be private
  double d[D];
public:
  double& operator[](size_t i) {return d[i];}
  double operator[](size_t i) const {return d[i];}
  Vec operator+(const Vec&) const; 
  Vec operator-(const Vec&) const; 
  Vec operator-() const; 
  bool operator==(const Vec& x) const;
};
  

template <> bool Vec<3>::operator==(const Vec& x) const
{return d[0]==x[0] && d[1]==x[1] && d[2]==x[2];}


template <> Vec<3> Vec<3>::operator-() const
{
  Vec r;
  r[0]=-d[0];
  r[1]=-d[1];
  r[2]=-d[2];
  return r;
}

template <> Vec<3> Vec<3>::operator+(const Vec& x) const
{
  Vec r;
  r[0]=d[0]+x[0];
  r[1]=d[1]+x[1];
  r[2]=d[2]+x[2];
  return r;
}

template <> Vec<3> Vec<3>::operator-(const Vec& x) const
{
  Vec r;
  r[0]=d[0]-x[0];
  r[1]=d[1]-x[1];
  r[2]=d[2]-x[2];
  return r;
}

template <> bool Vec<2>::operator==(const Vec& x) const
{return d[0]==x[0] && d[1]==x[1];}


template <> Vec<2> Vec<2>::operator-() const
{
  Vec r;
  r[0]=-d[0];
  r[1]=-d[1];
  return r;
}

template <> Vec<2> Vec<2>::operator+(const Vec& x) const
{
  Vec r;
  r[0]=d[0]+x[0];
  r[1]=d[1]+x[1];
  return r;
}

template <> Vec<2> Vec<2>::operator-(const Vec& x) const
{
  Vec r;
  r[0]=d[0]-x[0];
  r[1]=d[1]-x[1];
  return r;
}

class jellyfish
{
  CLASSDESC_ACCESS(jellyfish);
  void change_dir();
  bool collision(GraphID_t cell);
public:
  Vec<DIM> pos, vel;
  double radius;
  int colour; /* 0=black, 1=red, 2=green ... - see jellyfish::draw() */
  unsigned id;
  jellyfish(bool init=false) {if (init) jinit();}

  // uncomment for debugging!!
  //jellyfish() {assert(false);}

  /* accessors */
  double x() const {return pos[0];}
  double y() const {return pos[1];}
  double vx() const {return vel[0];}
  double vy() const {return vel[1];}
  double z() const {return pos[DIM-1];}
  double vz() const {return vel[DIM-1];}

  void jinit();  /* called when you need to initialise a jellyfish */
  void update();
  void draw(eco_string&,int);
  bool operator==(const jellyfish& j) const {
    return j.pos==pos && j.vel==vel && j.radius==radius&&
      j.colour==colour;
  }
  inline bool in_shadow(); 
  inline GraphID_t mapid();
  inline GraphID_t mapid_next();
};



class jlist: public vector<classdesc::ref<jellyfish> >, 
             virtual public classdesc::Object<jlist, GRAPHCODE_NS::object>
{
public:
  virtual void TCL_obj(const eco_string& d) {::TCL_obj(null_TCL_obj,d,*this);}

  clock_t etime; //CPU time used last update
  virtual idxtype weight() const 
    //  {idxtype s=size(); return /*s*s+*/1;}
  {return etime;}
  virtual idxtype edgeweight(const objref& x) const {
    const jlist *j=x.nullref()? 0: dynamic_cast<const jlist*>(&*x);
    return j? j->size()+1: 1;
  }
  jlist(): etime(1) {}
  jlist(const jlist& x): classdesc::Object<jlist, GRAPHCODE_NS::object>(x), vector<classdesc::ref<jellyfish> >(x), etime(x.etime) {}

  /* utility access routines  */
  typedef vector<classdesc::ref<jellyfish> >::iterator iterator;
  iterator begin() {return vector<classdesc::ref<jellyfish> >::begin();}
  iterator end() {return vector<classdesc::ref<jellyfish> >::end();}
  int size() const {return vector<classdesc::ref<jellyfish> >::size();}
  void clear() {vector<classdesc::ref<jellyfish> >::clear();}

  double mindist(const jellyfish& j);
  void add_jelly(classdesc::ref<jellyfish>& j) {vector<classdesc::ref<jellyfish> >::push_back(j);}
  void add_jelly(const jellyfish& j) {vector<classdesc::ref<jellyfish> >::push_back(classdesc::ref<jellyfish>(j));}
  void del_jelly(const iterator& j) {vector<classdesc::ref<jellyfish> >::erase(j);}
};

void swap(jlist& x, jlist& y) 
{swap((vector<classdesc::ref<jellyfish> >&)x, (vector<classdesc::ref<jellyfish> >&)y);}

inline void swapv(vector<classdesc::ref<jellyfish> >& x, vector<classdesc::ref<jellyfish> >& y) {swap(x,y);}
inline void asgv(jlist& x, jlist& y) 
{
  /* assign values, not just references */
  x.clear(); 
  for (jlist::iterator i=y.begin(); i!=y.end(); i++) 
    x.add_jelly(**i);
}

class jelly_map_t: public GRAPHCODE_NS::Graph
{
public:
  void init_map(unsigned,unsigned,unsigned);
  jlist& cell(GraphID_t ID) {return dynamic_cast<jlist&>(*objects[ID]);}
  static const jlist& cell(const objref& o) {return dynamic_cast<const jlist&>(*o);}
  static jlist& cell(objref& o) {return dynamic_cast<jlist&>(*o);}
  /* copy actual jellyfish, not references */
  void copy(const jelly_map_t& x)
  {
    assert(x.size()==size());
    for (iterator i=begin(), j=x.begin(); i!=end(); i++, j++)
      asgv(dynamic_cast<jlist&>(**i),dynamic_cast<jlist&>(**j));
  }
  size_t max_size();
  /* integer function used in init_map */
  class mapid_t
  {
  public:
    unsigned nmapx, nmapy, nmapz;
    mapid_t(unsigned x, unsigned y, unsigned z): nmapx(x), nmapy(y), nmapz(z) {}
    GraphID_t operator()(int i, int j)
    {
      if (i<0 || i>=int(nmapx) || j<0 || j>=int(nmapy)) return bad_ID;
      else return i+nmapx*j;
    }

    GraphID_t operator()(int i, int j, int k)
    {
      if (i<0 || i>=int(nmapx) || j<0 || j>=int(nmapy) || k<0 || k>=int(nmapz)) 
	return bad_ID;
      else return i+nmapx*(j+nmapy*k);
    }
  };
  //    void Partition_Objects();
 };

size_t jelly_map_t::max_size() {size_t m=0; for (iterator i=begin(); i!=end(); i++) m=(m>(*i)->size())? m: (*i)->size(); return m;}

class lake_t: public TCL_obj_t
{
  CLASSDESC_ACCESS(lake_t);
  ecolab::array<int> bdy;
  ecolab::array<int> edge_indices;
  bool shadow_updated;
public:
  unsigned width, height, depth, nmapx, nmapy, nmapz;
  unsigned contrast;
  double mangrove_height;
  /* normal components of Sun's direction */
  double sx, sy, sz; 
  jelly_map_t jelly_map;
  map<GraphID_t,vector<jellyfish> > back_map;

  lake_t(): width(0), height(0), mangrove_height(0) {}

  /* 2D mapid function */
  GraphID_t mapid(double x, double y) 
  {return unsigned((x*nmapx)/width)+nmapx*(unsigned((y*nmapy)/height));}


  /* 3D mapid function */
  GraphID_t mapid(double x, double y, double z) 
  {return unsigned((x*nmapx)/width)+nmapx*(unsigned((y*nmapy)/height)+
					 nmapy*unsigned((z*nmapz)/depth));}
  
   
  void clear_non_local();
   
  bool in_water(int x, int y) 
  {return x>0 && x<int(width)-1 && y>0 && y<int(height)-1 && bdy[x+width*y];} 
  bool in_water(const Vec<2>& p) {return in_water(p[0],p[1]);}
  bool in_water(const Vec<3>& p) {
    return p[2]<=0 && -p[2]<depth && in_water(p[0],p[1]);}
  bool is_lake_edge(int x, int y) 
  {
    if (x>1 && x<int(width)-2 && y>1 && y<int(height)-2)
      return bdy[x+width*y] && !(bdy[x-1+width*y] && bdy[x+1+width*y] && 
				 bdy[x+width*(y-1)] && bdy[x+width*(y+1)]);
    else return in_water(x,y);
  } 
  bool in_shadow(int x, int y);
  bool in_shadow(const Vec<2>& p);
  bool in_shadow(const Vec<3>& p);
  void set_lake(TCL_args);
  void init_map(TCL_args);
  void gather();
  void compute_shadow(TCL_args);
  void add_jellyfish(TCL_args);
  void update(TCL_args);
  void update_map(TCL_args);
  void draw(TCL_args);
  void draw_density(TCL_args);
  eco_string select(TCL_args);
  ecolab::array<double> depthlist(TCL_args);
  int cell_max(TCL_args);
};

/* 
   data structures for partitioner 
   operator< allows (reverse) sort to be applied to vectors of these things
*/

/* amount of work assigned to processor proc */
struct part_wt_t
{
  unsigned proc, weight;
  bool operator<(const part_wt_t& x) const {return weight>x.weight;}
};

/* amount of work assigned to cell ID */
struct cell_wt_t
{
  GraphID_t ID;
  unsigned weight;
  cell_wt_t() {}
  cell_wt_t(const cell_wt_t& x): ID(x.ID), weight(x.weight) {}
  cell_wt_t(GraphID_t i,unsigned w): ID(i), weight(w) {}
  bool operator<(const cell_wt_t& x) const {return weight>x.weight;}
};  

