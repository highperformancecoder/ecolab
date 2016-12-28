#include "ecolab.h"
using namespace ecolab;
#define MAP vmap
#include "graphcode.h"
#include "graphcode.cd"
using namespace GRAPHCODE_NS;
#include "ref.h"
#include "stupid12.h"
#include "stupid12.cd"
#include "pack_graph.h"
#include "ecolab_epilogue.h"

#include <sstream>
#include <iomanip>

StupidModel stupidModel;
make_model(stupidModel);

/* use bugArch to set initial state of created bugs */
StupidBug bugArch;
make_model(bugArch);

inline Cell* getCell(GraphID_t id) {return getCell(stupidModel.objects[id]);}
inline double ran() {return stupidModel.u.rand();}

int StupidBug::x() {return getCell(cellID)->x;}
int StupidBug::y() {return getCell(cellID)->y;}

inline void StupidBug::move()
{
  int newX, newY, nbr_idx;
  objref* newCellref;
  Cell* newCell=getCell(cellID), *myCell=newCell;
  double best_food=myCell->food_avail;
  // find an unoccupied cell in the neighbourhood
  for ( Ptrlist::iterator nbr=myCell->begin(); nbr!=myCell->end(); nbr++)
    {
      Cell* nCell=getCell(*nbr);
      if (!nCell->occupied() && nCell->food_avail>best_food)
	{
	  newCell=nCell;
	  newCellref=&*nbr;
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
  

void Cell::grow_food()
{
  if (food_avail < max_food) 
    food_avail+=stupidModel.u.rand()*max_food_production;
}

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
	o->push_back(o); //self is first reference on neigbourhood list
	/* connect up a square neighbourhood of size 2*moveDistance+1 */
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
      bugs.push_back(cell->bug[0]);
    }
}

struct BugMore
{
  bool operator()(const classdesc::ref<StupidBug>& x, const classdesc::ref<StupidBug>& y) 
  {
    return x->size > y->size;
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
  double incr=std::min(max_consumption,cell->food_avail); 
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

void StupidModel::draw(TCL_args args)
{
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete bugs\n";
  for (int i=0; i<bugs.size(); i++)
    bugs[i]->draw(canvas);
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
      TCL_obj(null_TCL_obj,id.str(),*cell.bug[0]);
    }
  else
    {
      id | "cell" | cnt++;
      TCL_obj(null_TCL_obj,id.str(),cell);
    }
  return id.str();
}
