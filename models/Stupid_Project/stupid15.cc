#include "ecolab.h"
#define MAP vmap
#include "graphcode.h"
#include "graphcode.cd"
using namespace GRAPHCODE_NS;
#include "ref.h"
#include "stupid15.h"
#include "stupid15.cd"
#include "pack_graph.h"
#include "ecolab_epilogue.h"

#include <sstream>
#include <iomanip>

void Cell::lpack(pack_t *buf) {pack(buf,"",*this);}
void Cell::lunpack(pack_t *buf) {unpack(buf,"",*this);}

StupidModel stupidModel;
make_model(stupidModel);

/* use bugArch to set initial state of created bugs */
StupidBug bugArch;
make_model(bugArch);

inline Cell* getCell(GraphID_t id) 
{
  assert(id==(*getCell(stupidModel.objects[id]))[0].ID);
  return getCell(stupidModel.objects[id]);
}
inline double ran() {return stupidModel.u.rand();}

int StupidBug::x() {return getCell(cellID)->x;}
int StupidBug::y() {return getCell(cellID)->y;}

inline void StupidBug::move()
{
  int newX, newY, nbr_idx;
  objref* newCellref;
  Cell* newCell=getCell(cellID), *myCell=newCell;
  double best_food=myCell->food_avail;

  // shuffle neighbourhood list, leaving the first element point to bug's
  // current cell
  //  random_shuffle(myCell->begin()+1,myCell->end());
  // the above did not work, because Ptrlist::iterator doesn't contain 
  // iterator_category (and other similar properties)
  //copy objref pointers into a temporary vector, then shuffle
  vector <objref*> neighbourlist(myCell->size());
  int i=0;
  for (Ptrlist::iterator nbr=myCell->begin(); nbr!=myCell->end(); nbr++)
    neighbourlist[i++]=&*nbr;
  random_shuffle(neighbourlist.begin()+1,neighbourlist.end());

  // find an unoccupied cell in the neighbourhood
  for ( vector <objref*>::iterator nbr=neighbourlist.begin(); 
	nbr!=neighbourlist.end(); nbr++)
    {
      Cell* nCell=getCell(**nbr);
      if (!nCell->occupied() && nCell->food_avail>best_food)
	{
	  newCell=nCell;
	  newCellref=*nbr;
	  best_food=newCell->food_avail;
	}
    }
  if (newCell==myCell)  //no better unoccupied neighbours
    return;  
  // swap myself to new cell
  newCell->moveBug(*myCell);
  cellID=newCellref->ID;
}

void StupidBug::die() 
{getCell(cellID)->bug.clear();}

/* return true if bug dies */
inline bool StupidBug::mortality()
{
  Cell &myCell=*getCell(cellID);
  if (size>repro_size)
    {
      for (int i=0; i<5; i++) // generate 5 new bugs
	for (int j=0; j<5; j++) // 5 goes at placement
	  {
	    objref& birth_ref=myCell[stupidModel.u.rand()*myCell.size()];
	    Cell& birth_loc=*getCell(birth_ref);
	    if (!birth_loc.occupied())
	      {
		birth_loc.addBug(bugArch);
		stupidModel.bugs.push_back(birth_loc.bug.back());
		break;
	      }
	  }
      return true;
    }

  if (stupidModel.u.rand()>survivalProbability) return true;
  else return false;
}
  
Cell::Cell(unsigned x_, unsigned y_, double mfp):
  x(x_), y(y_), food_avail(0), food_production(mfp) {}


void Space::setup(int nx_, int ny_, int moveDistance, bool toroidal_ ,
		  double max_food_production)
{
  nx=nx_, ny=ny_;
  assert(moveDistance <= nx/2 && moveDistance <= ny/2 );
  toroidal=toroidal_;
  
  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      {
	objref& o=objects[mapid(i,j)];
	AddObject(Cell(),o.ID);
	*getCell(o)=Cell(i,j,max_food_production);
      }
  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      {
	objref& o=objects[mapid(i,j)];
	/* connect up a square neighbourhood of size 2*moveDistance+1 */
	o->push_back(o); //self is first reference on neigbourhood list
	for (int ii=-moveDistance; ii<=moveDistance; ii++)
	  for (int jj=-moveDistance; jj<=moveDistance; jj++)
	    if (ii!=0 || jj!=0)
	      o->push_back(objects[mapid(i+ii,j+jj)]);
      }
  rebuild_local_list();
}

GraphID_t Space::mapid(int x, int y)
{
  assert(x>=-nx && y>=-ny);
  if (toroidal)    
    {
      /* place x and y into [0..nx,ny) */
      if (x<0 || x>=nx) 
	x=(x+nx)%nx;
      if (y<0 || y>=ny) 
	y=(y+ny)%ny;
    }
  else
    if (x<0 || x>=nx || y<0 || y>=ny)
      return bad_ID;
  return x+nx*y;
}
    
void StupidModel::addBugs(TCL_args args)
{
  int  nBugs=args;
  Cell *cell;
  for (int i=0; i<nBugs; i++)
    {
      do
	{
	  cell=randomCell();
	} while (cell->occupied());
      cell->addBug(bugArch);
      /* initialise bug to a random size accrdoing to initBugDist */
      double sz=initBugDist->rand();
      cell->bug[0]->size=(sz>0)? sz: 0;
      bugs.push_back(cell->bug[0]);
    }
}

struct BugMore
{
  bool operator()(const ref<StupidBug>& x, const ref<StupidBug>& y) 
  {
    /* ref does not have const accessor method, hence the fugly casts */ 
    return const_cast<ref<StupidBug>&>(x)->size >
     const_cast<ref<StupidBug>&>(y)->size;
  }
};

void StupidModel::moveBugs()
{
  sort(bugs.begin(),bugs.end(),BugMore());
  for (int i=0; i<bugs.size(); i++)
    bugs[i]->move();
  tstep++;
}

void StupidModel::birthdeath()
{
  vector<int> deathlist;
  for (int i=0; i<bugs.size(); i++)
    if (bugs[i]->mortality())
      deathlist.push_back(i);
  for (int i=deathlist.size()-1; i>=0; i--)
    {
      bugs[deathlist[i]]->die();
      bugs.erase(bugs.begin()+deathlist[i]);
    }
}

void StupidBug::grow()
{
  Cell *cell=getCell(cellID);
  double incr=min(max_consumption,cell->food_avail); 
  size+=incr;
  cell->food_avail-=incr;
}

inline char hexdigit(int i)
{ assert(i<16); return i<10? i+'0': i+'a'-10;}

void StupidBug::draw(const eco_string& canvas)
{
  tclcmd c;
  char h[3];
  int v = size<10? 255-int(size*25.6): 0;
  h[0]=hexdigit(v/16);
  h[1]=hexdigit(v%16);
  h[2]='\0';
  int scale=stupidModel.scale;
  c << canvas << "create rectangle" << (scale*x()) << (scale*y()) << 
    (scale*(x()+1)) << (scale*(y()+1));
  c |" -fill #ff"|h|h;
  c << "-tags bugs\n";
}

void Cell::draw(const eco_string& canvas)
{
  assert((*this)[0].ID==stupidModel.mapid(x,y));
  tclcmd c;
  char h[3];
  int v = food_avail<0.5? int(food_avail*512.0): 255;
  h[0]=hexdigit(v/16);
  h[1]=hexdigit(v%16);
  h[2]='\0';
  int scale=stupidModel.scale;
  c << canvas << "create rectangle" << (scale*x) << (scale*y) << 
    (scale*(x+1)) << (scale*(y+1));
  c |" -fill #00"|h|"00";
  c << "-tags cells\n";
}

void StupidModel::drawBugs(TCL_args args)
{
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete bugs\n";
  for (int i=0; i<bugs.size(); i++)
    bugs[i]->draw(canvas);
}

void StupidModel::drawCells(TCL_args args)
{
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete cells\n";
  for (omap::iterator i=objects.begin(); i!=objects.end(); i++)
    getCell(*i)->draw(canvas);
}

void StupidModel::grow()
{
  for (Ptrlist::iterator i=begin(); i!=end(); i++)
    {
      Cell *cell=getCell(*i);
      cell->grow_food();
      if (cell->bug.size()>0)
	cell->bug[0]->grow();
    }
}

/** return a TCL object representing a bug 
    (if one exists at that location, and bug passed as third parameter) */

eco_string StupidModel::probe(TCL_args args)
{
  int x=args, y=args;
  x/=scale; y/=scale; //reduce to application coordinates
  bool bug = strcmp(args,"bug")==0;
  eco_strstream id;
  static int cnt=0;
  Cell& cell=*getCell(*getObjAt(x,y));
  if (bug && cell.occupied())
    {
      id | "bug" | cnt++;
      TCL_obj(NULL,id,*cell.bug[0]);
    }
  else
    {
      id | "cell" | cnt++;
      TCL_obj(NULL,id,cell);
    }
  return id;
}

struct Prod_store
{
  int x,y;
  double p;
  Prod_store(int x,int y,double p): x(x), y(y), p(p) {}
};

void StupidModel::read_food_production(TCL_args args)
{
  char* filename=args;
  int moveDistance=args;

  FILE *food_input=fopen(filename,"r");
  /* discard first 3 lines */
  for (int i=0; i<3; i++)
    for (int c=fgetc(food_input); c!='\n' && c!=EOF; c=fgetc(food_input));

  int x, y, nx=0, ny=0;
  double prod;

  vector<Prod_store> prod_store;

  while (fscanf(food_input,"%d %d %lg",&x,&y,&prod)==3)
    {
      nx=max(nx,x+1); ny=max(ny,y+1);
      prod_store.push_back(Prod_store(x,y,prod));
    }

  fclose(food_input);

  Space::setup(nx,ny,moveDistance,false,0.1);
  for (vector<Prod_store>::iterator i=prod_store.begin(); i!=prod_store.end(); i++)
    getCell(*getObjAt(i->x,i->y))->food_production=i->p;
}

void StupidModel::killBug(ref<StupidBug>& bug)
{
  bugs.erase(find(bugs.begin(),bugs.end(),bug));
  bug->die();
}

