#include "arrays.h"

class StupidBug
{
  GraphID_t cellID;  // Graphcode ID of cell where this bug is located
  CLASSDESC_ACCESS(StupidBug);
public:
  int x(); 
  int y(); 
  double size;
  double max_consumption;
  StupidBug(GraphID_t i=-1, double m=1): cellID(i), size(1), 
					 max_consumption(m) {}
  void move();
  void grow(); 
  void draw(const eco_string& canvas);
};


class Cell: public Object<Cell,GRAPHCODE_NS::object>
{
  CLASSDESC_ACCESS(Cell);
public:
  unsigned x,y;
  double food_avail, max_food_production, max_food;
  vector<classdesc::ref<StupidBug> > bug;
  bool occupied() {return bug.size()>0;}
  Cell() {}
  Cell(unsigned x_, unsigned y_, double mfp=0.01):x(x_), y(y_), food_avail(0), 
						  max_food_production(mfp) {}
  Cell(const StupidBug& b) {bug.push_back(b);}
  void addBug(const StupidBug& b) {bug.resize(1); *bug[0]=b;}
  void moveBug(Cell& from) { 
    assert(from.size()>0);
    if (&from==this) return;
    assert(!occupied());
    bug=from.bug; 
    from.bug.clear(); 
  }
  
  void grow_food();
};

/* casting utilities */
inline Cell* getCell(objref& x) {return dynamic_cast<Cell*>(&*x);}
inline const Cell* getCell(const objref& x) {return dynamic_cast<const Cell*>(&*x);}

/* casting utilities */
inline StupidBug* getBug(objref& x)   {return dynamic_cast<StupidBug*>(&*x);}
inline const StupidBug* getBug(const objref& x)   {return dynamic_cast<const StupidBug*>(&*x);}

class Space: public Graph
{
  bool toroidal;
  CLASSDESC_ACCESS(Space);
 public:
  int nx, ny;  //dimensions of space
  urand u;
  Space(): nx(0), ny(0) {}
  void setup(int nx, int ny, int moveDistance, bool toroidal, 
	     double max_food_production);
  GraphID_t mapid(int x, int y);
  objref* getObjAt(int x, int y) {return &objects[mapid(x,y)];}
  Cell* randomCell() {return getCell(*getObjAt(u.rand()*nx,u.rand()*ny));}
};

class StupidModel: public Space
{
  CLASSDESC_ACCESS(StupidModel);
public:
  urand u;     //random generator for positions
  int tstep;   //timestep - updated each time moveBugs is called
  int scale;   //no. pixels used to represent bugs
  vector<classdesc::ref<StupidBug> > bugs; 
  void setup(TCL_args args) {
    int nx=args, ny=args, moveDistance=args;
    bool toroidal=args;
    double max_food_production=args;
    Space::setup(nx,ny,moveDistance,toroidal,max_food_production);
  }
  // addBugs(int nBugs);
  void addBugs(TCL_args args);
  void moveBugs();
  void draw(TCL_args args);
  void grow();
  /** return a TCL object representing a bug 
      (if one exists at that location, and bug passed as third paramter) */
  eco_string probe(TCL_args); 
  ecolab::array<double> bugsizes() {
    ecolab::array<double> r;
    for (size_t i=0; i<bugs.size(); i++)
      r <<= bugs[i]->size;
    return r;
  }
  double max_bugsize() {
    double r=0;
    for (int i=0; i<bugs.size(); i++)
      r = std::max(bugs[i]->size,r);
    return r;
  }
};
