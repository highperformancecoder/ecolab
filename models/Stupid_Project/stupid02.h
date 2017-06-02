class StupidBug
{
  GraphID_t cellID;  // Graphcode ID of cell where this bug is located
  CLASSDESC_ACCESS(StupidBug);
public:
  int x(); 
  int y(); 
  unsigned size;

  StupidBug(GraphID_t i=-1): cellID(i), size(1) {}
  void move();
  void grow() {size++;}
  void draw(const eco_string& canvas);
};


class Cell: public Object<Cell,GRAPHCODE_NS::object>
{
  CLASSDESC_ACCESS(Cell);
public:
  unsigned x,y;
  vector<StupidBug> bug;
  bool occupied() {return bug.size()>0;}
  Cell() {}
  Cell(unsigned x_, unsigned y_):x(x_), y(y_) {}
  void addBug(const StupidBug& b) {bug.resize(1); bug[0]=b;}
  void moveBug(Cell& from) { 
    assert(from.size()>0);
    bug=from.bug; 
    from.bug.clear(); 
  }
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
  void setup(int nx, int ny, int moveDistance, bool toroidal);
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
    Space::setup(nx,ny,moveDistance,toroidal);
  }
  // addBugs(int nBugs);
  void addBugs(TCL_args args);
  void moveBugs();
  void draw(TCL_args args);
  void grow();
};

