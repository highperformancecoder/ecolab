#include "ecolab.h"
using namespace ecolab;
#define MAP vmap
#include "graphcode.h"
#include "graphcode.cd"
using namespace GRAPHCODE_NS;
#include "ref.h"
#include "stupid01.h"
#include "stupid01.cd"
#include "ecolab_epilogue.h"

StupidModel stupidModel;
make_model(stupidModel);

inline Cell* getCell(GraphID_t id) {return getCell(stupidModel.objects[id]);}
inline double ran() {return stupidModel.u.rand();}

int StupidBug::x() {return getCell(cellID)->x;}
int StupidBug::y() {return getCell(cellID)->y;}

inline void StupidBug::move()
{
  int newX, newY, nbr_idx;
  objref* newCellref;
  Cell* newCell, *myCell=getCell(cellID);
  // find an unoccupied cell in the neighbourhood
  do 
	{
	  newCellref=&(*myCell)[ran()*myCell->size()];
	  newCell=getCell(*newCellref);
	} while (newCell->occupied());
  // swap myself to new cell
  newCell->moveBug(*myCell);
  cellID=newCellref->ID;
}

void Space::setup(int nx_, int ny_, int moveDistance, bool toroidal_)
{
  nx=nx_, ny=ny_;
  assert(moveDistance <= nx/2 && moveDistance <= ny/2 );
  toroidal=toroidal_;
  
  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      {
	objref& o=objects[mapid(i,j)];
	AddObject(Cell(),o.ID);
	*getCell(o)=Cell(i,j);
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
      cell->addBug(StupidBug(cell->begin()->ID));
      bugs.push_back(cell->bug[0]);
    }
}

void StupidModel::moveBugs()
{
  for (size_t i=0; i<bugs.size(); i++)
    bugs[i]->move();
  tstep++;
}
void StupidBug::draw(const eco_string& canvas)
{
  tclcmd c;
  int scale=stupidModel.scale;
  c << canvas << "create rectangle" << (scale*x()) << (scale*y()) << 
    (scale*(x()+1)) << (scale*(y()+1)) <<"-fill red -tags bugs\n";
}

void StupidModel::draw(TCL_args args)
{
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete bugs\n";
  for (size_t i=0; i<bugs.size(); i++)
    bugs[i]->draw(canvas);
}
