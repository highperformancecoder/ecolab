#include <oldarrays.h>

extern urand u;  //system random no. generator

class Space: public Graph
{
  /* first cell of processor i */   
  int cellOffset(int i)
  {return (i*(nx/nprocs) + (i<nx%nprocs? i: nx%nprocs))*ny;}

  /* No.  cell of processor i */   
  int nCells(int i)
  {return (nx/nprocs + (i<nx%nprocs))*ny;}

  CLASSDESC_ACCESS(Space);
public:
  int nx, ny;
  bool toroidal;
  int apron;   //size of overlap area between cells
  int scale;   //no. pixels used to represent bugs
  int offs, size;  //ID of first cell, and no cells on this process
  GraphID_t mapid(int x, int y);
  vector <double> food_avail, food_production;
  void grow_food() {
    for (int i=0; i<food_avail.size(); i++)
      food_avail[i]+=food_production[i];
  }
  void setup(int nx, int ny, int nbrhdSz, bool toroidal, double prod); 
  void draw(const eco_string& canvas);

  /** parallel support */
  void gather();
  void repartition();
};

extern Space &space;

class StupidBug;
class Predator;

class Cell: public object
{
  GraphID_t cellID;
  int _x,_y;
  friend void Space::setup(int,int,int,bool,double); 
  CLASSDESC_ACCESS(Cell);
public:
  double& food_avail() {return space.food_avail[long(cellID)-space.offs];}
  double& food_production() {return space.food_production[long(cellID)-space.offs];}
  double food_avail() const {return space.food_avail[long(cellID)-space.offs];}
  double food_production() const {return space.food_production[long(cellID)-space.offs];}
  double Food_avail() const {return food_avail();}
  double Food_production() const {return food_production();}
  GraphID_t ID() {return cellID;}
  int x() {return _x;}
  int y() {return _y;}

  Cell() {}
  Cell(int i, int j): _x(i), _y(j), cellID(space.mapid(i,j)) {}

  ref<StupidBug> bug;
  ref<Predator> predator;
  Ptrlist bugNbrhd, predatorNbrhd;

  /* override virtual methods of object */
  void lpack(pack_t *buf);
  void lunpack(pack_t *buf);
  object* lnew() const {return vnew(this);}
  object* lcopy() const {return vcopy(this);}
  int type() const {return vtype(*this);}

};

Cell& getCell(GraphID_t ID) {return dynamic_cast<Cell&>(*space.objects[ID]);}
Cell& getCell(objref& o) {return dynamic_cast<Cell&>(*o);}

class Anim  //base class for stupidBug and Predator
{
protected:
  GraphID_t cellID;
  CLASSDESC_ACCESS(Anim);
public:
  int x() {return getCell(cellID).x();}
  int y() {return getCell(cellID).y();}
  Cell& cell() {return getCell(cellID);}
  Anim(): cellID(-1) {}
};

class StupidBug: public Anim
{
public:
  double size;
  double repro_size;
  double max_consumption;
  double survivalProbability;
  
  void moveTo(Cell& dest) {
    Cell& src=cell();
    dest.bug=src.bug;
    src.bug.nullify();
    dest.bug->cellID=dest.ID();
  }

  void copyTo(Cell& dest) {
    dest.bug=*this;
    dest.bug->cellID=dest.ID();
  }

  void move();
  void grow(); 
  void die() {cell().bug.nullify();}
  bool mortality();
  void draw(const eco_string& canvas);

};

class Predator: public Anim
{
public:
  void moveTo(Cell& dest) {
    Cell& src=cell();
    dest.predator=src.predator;
    src.predator.nullify();
    dest.predator->cellID=dest.ID();
  }

  void copyTo(Cell& dest) {
    dest.predator=*this;
    dest.predator->cellID=dest.ID();
  }
  void hunt();
  void draw(const eco_string& canvas);

};

class StupidModel: public Space, TCL_obj_t
{
public:
  int tstep;   //timestep - updated each time moveBugs is called
  vector<ref<StupidBug> > bugs; 
  vector<ref<Predator> > predators; 
  random_gen *initBugDist; //Initial distribution of bug sizes
  vector<vector< pair<GraphID_t,GraphID_t> > > emmigration_list;

  StupidModel(): initBugDist(&u) {}
  void setup(TCL_args args) {
    parallel(args);
    int nx=args, ny=args, moveDistance=args;
    bool toroidal=args;
    double prod=args;
    Space::setup(nx,ny,moveDistance,toroidal,prod);
  }
  // addBugs(int nBugs);
  void addBugs(TCL_args args);
  void addPredators(TCL_args args);
  void moveBugs(TCL_args);
  void birthdeath(TCL_args);
  void grow(TCL_args);
  void killBug(ref<StupidBug>& bug);
  void hunt(TCL_args);

  /** visualisation routines */
  void drawBugs(TCL_args args);
  void drawPredators(TCL_args args);
  void drawCells(TCL_args args);

  void gather(TCL_args args) {
    parallel(args);
    Space::gather();
  }

  /*
    EcoLab's checkpoint/restart only supports scalar mode - supply parallel 
    versions using scalar version as base
  */
  void checkpoint(TCL_args args) { //* force explicit gather
    parallel(args);
    Space::gather();
    if (myid==0)
      {
        pack_t ckpt((char*)args,"w");
        ckpt << *this;
      }
//    eco_string es=args;
//    char *fname[2]={NULL,es};
//    if (myid==0) TCL_obj_t::checkpoint(2,fname); //nyarggh - need a TCL_args version
  }
  void restart(TCL_args args) {
    parallel(args);
//    eco_string es=args;
//    char *fname[2]={NULL,es};
//    if (myid==0) TCL_obj_t::restart(2,fname); //nyarggh - need a TCL_args version
    if (myid==0)
      {
        pack_t ckpt((char*)args,"r");
        ckpt>>*this;
      }
    repartition();
    /* relink bugs and predators */
//    for (int i=0; i<bugs.size(); i++)
//      bugs[i]=bugs[i]->cell().bug;
//    for (int i=0; i<predators.size(); i++)
//      predators[i]=predators[i]->cell().predator;
    bugs.clear(); predators.clear();
    for (Ptrlist::iterator i=begin(); i!=end(); i++)
      {
	if (getCell(*i).bug)
	  bugs.push_back(getCell(*i).bug);
	if (getCell(*i).predator)
	  predators.push_back(getCell(*i).predator);
      }
  }

  /** return a TCL object representing a bug 
      (if one exists at that location, and bug passed as third paramter) */
  eco_string probe(TCL_args); 
  array bugsizes() {
    array r;
    for (int i=0; i<bugs.size(); i++)
      r <<= bugs[i]->size;
    return r;
  }

  double max_bugsize(TCL_args args) {
    parallel(args);
    double r=-1;
    for (int i=0; i<bugs.size(); i++)
      r = std::max(bugs[i]->size,r);
#ifdef MPI_SUPPORT
    double r1;
    MPI_Reduce(&r,&r1,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
    return r1;
#else
    return r;
#endif
  }
  void read_food_production(TCL_args);
};

/**
   Used for temporary storage of food production data in read_food_production
*/

struct Prod_store
{
  int x,y;
  double p;
  Prod_store(int x,int y,double p): x(x), y(y), p(p) {}
  Prod_store() {}
};

