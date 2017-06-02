#include "ecolab.h"
using namespace ecolab;
using namespace std;
#define MAP vmap
#include "graphcode.h"
#include "graphcode.cd"
using GRAPHCODE_NS::Ptrlist;
using GRAPHCODE_NS::objref;
using GRAPHCODE_NS::object;
using GRAPHCODE_NS::omap;
using GRAPHCODE_NS::Graph;
using GRAPHCODE_NS::GraphID_t;
using GRAPHCODE_NS::bad_ID;

#include "ref.h"
#include "stupid-parallel.h"
#include "stupid-parallel.cd"
#include "ecolab_epilogue.h"

#include <classdescMP.h>
using classdesc::MPIbuf;
using classdesc::bcast;

#include <sstream>
#include <iomanip>

StupidModel stupidModel;
make_model(stupidModel);

/* use bugArch to set initial state of created bugs */
StupidBug bugArch;
make_model(bugArch);

/* use predatorArch to set initial state of created predators */
Predator predatorArch;
make_model(predatorArch);

inline Cell* getCell(GraphID_t id) 
{
  assert(id==(*getCell(stupidModel.objects[id]))[0].ID);
  return getCell(stupidModel.objects[id]);
}

inline int ran(int n) {return n*stupidModel.u.rand();}

int StupidBug::x() {return getCell(cellID)->x;}
int StupidBug::y() {return getCell(cellID)->y;}

int Predator::x() {return getCell(cellID)->x;}
int Predator::y() {return getCell(cellID)->y;}

inline void StupidBug::move()
{
  int newX, newY, nbr_idx;
  objref* newCellref=NULL;
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
  for (Ptrlist::iterator nbr=myCell->bugNbrhd.begin(); nbr!=myCell->bugNbrhd.end(); nbr++)
    neighbourlist[i++]=&*nbr;
  random_shuffle(neighbourlist.begin()+1,neighbourlist.end(),ran);

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
#ifdef MPI_SUPPORT
  //preferential treatment to on-processor moves
  if (newCellref->proc!= ::myid)
    //request emmigration
    stupidModel.emmigration_list[newCellref->proc].
      push_back(pair<GraphID_t,GraphID_t>(cellID,newCellref->ID)); 
  else
#endif
    {
      // swap myself to new cell
      newCell->moveBug(*myCell);
      cellID=newCellref->ID;
    }
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
	    objref& birth_ref=myCell.bugNbrhd[stupidModel.u.rand()*myCell.size()];
	    Cell& birth_loc=*getCell(birth_ref);
	    if (!birth_loc.occupied() && birth_ref.proc==myid)
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
	o.proc=(i*nprocs)/nx;
	if (o.proc==myid)
	  {
	    AddObject(Cell(),o.ID);
	    *getCell(o)=Cell(i,j,max_food_production);
	  }
      }
  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      {
	objref& o=objects[mapid(i,j)];
	if (o.proc==myid)
	  {
	    /* connect up a square neighbourhood of size 2*moveDistance+1 */
	    o->push_back(o); //self is first reference on neigbourhood list
	    for (int ii=-moveDistance; ii<=moveDistance; ii++)
	      for (int jj=-moveDistance; jj<=moveDistance; jj++)
		if (ii!=0 || jj!=0)
		  o->push_back(objects[mapid(i+ii,j+jj)]);
	    //Set up bug and predator neighbourhood lists
	    Cell& cell=*getCell(o);
	    cell.bugNbrhd=cell;
	    // wire up predator neighbourhood - use Moore neighborhood, 
	    //even though specification is ambiguous
	    for (int ii=-1; ii<=1; ii++)
	      for (int jj=-1; jj<=1; jj++)
		cell.predatorNbrhd.push_back(objects[mapid(i+ii,j+jj)]);
	    assert(o.ID==mapid(cell.x,cell.y));
	  }
      }

  rebuild_local_list();
  Partition_Objects();
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
  parallel(args);
  int  nBugs=args;
  Cell *cell;
  for (int i=0; i<nBugs; i+=nprocs)
    {
      do
	{
	  cell=randomCell();
	} while ((*cell)[0].proc!=myid || cell->occupied());
      cell->addBug(bugArch);
      /* initialise bug to a random size according to initBugDist */
      double sz=initBugDist->rand();
      cell->bug[0]->size=(sz>0)? sz: 0;
      bugs.push_back(cell->bug[0]);
    }
  assert(unique(bugs));
}

void StupidModel::addPredators(TCL_args args)
{
  using ::nprocs;
  using ::myid;  
  parallel(args);
  int  nPredators=args;
  Cell *cell;
  for (int i=0; i<nPredators; i++)
    {
      do
	{
	  cell=randomCell();
	} while ((*cell)[0].proc!=myid || cell->predator.size()>0);
      cell->addPredator(predatorArch);
      predators.push_back(cell->predator[0]);
    }
}

struct BugMore
{
  bool operator()(const classdesc::ref<StupidBug>& x, const classdesc::ref<StupidBug>& y) 
  {
    return x->size > y->size;
  }
};

void StupidModel::moveBugs(TCL_args args)
{
  parallel(args);
  emmigration_list.clear();
  emmigration_list.resize(nprocs);
  Prepare_Neighbours();
  sort(bugs.begin(),bugs.end(),BugMore());

  for (size_t i=0; i<bugs.size(); i++)
      bugs[i]->move();

#ifdef MPI_SUPPORT
  // now process the emmigration list
  // firstly send a list of requested destinations to host processor
  MPIbuf_array migrants(nprocs), approved(nprocs);
  for (size_t i=0; i<nprocs; i++)
    if (i!=myid)
      {
        for (size_t j=0; j<emmigration_list[i].size(); j++)
          migrants[i]<<emmigration_list[i][j].second;
        migrants[i]<<isend(i,1);
      }

  // recieve requested destinations, and approve or deny immigration
  MPIbuf b;
  set<GraphID_t> takenCells; 
  for (size_t i=0; i<nprocs-1; i++)
    {
      GraphID_t dest;
      b.get(MPI_ANY_SOURCE,1);
      while (b)
        {
          b >> dest;
          if (!getCell(dest)->occupied() && !takenCells.count(dest))
            {
              takenCells.insert(dest);
              approved[b.proc]<<true;
            }
          else
              approved[b.proc]<<false;
        }
    }

  // return approval list to originator
  for (size_t i=0; i<nprocs; i++)
    if (i!=myid)
      approved[i]<<isend(i,2);

  // read in approval list
  vector<vector<bool> > immigration_approved(nprocs);
  for (size_t i=0; i<nprocs-1; i++)
    {
      bool approved;
      b.get(MPI_ANY_SOURCE,2);
      while (b)
        {
          b >> approved;
          immigration_approved[b.proc].push_back(approved);
        }
    }
  
  migrants.waitall();

  GraphID_t dest;
  for (size_t proc=0; proc<nprocs; proc++)
    {
      assert(emmigration_list[proc].size()==immigration_approved[proc].size());
      for (size_t i=0; i<emmigration_list[proc].size(); i++)
        {
          dest=emmigration_list[proc][i].second;
          if (immigration_approved[proc][i])
            {
              Cell *cell=getCell(emmigration_list[proc][i].first);
              migrants[proc]<<dest << *cell->bug[0];
              killBug(cell->bug[0]);
            }
        }
    }

  // send emmigrants
  for (size_t i=0; i<nprocs; i++)
    if (i!=myid) migrants[i].isend(i,3);
  //receive immigrants

  for (size_t i=0; i<nprocs-1; i++)
  {
    b.get(MPI_ANY_SOURCE,3);
    StupidBug immigrant;
    while (b)
      {
	b>>dest>>immigrant;
	getCell(dest)->addBug(immigrant);
	bugs.push_back(getCell(dest)->bug[0]);
      }
  }
#endif
  tstep++;
}

void StupidModel::birthdeath(TCL_args args)
{
  vector<int> deathlist;
  for (size_t i=0; i<bugs.size(); i++)
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

void Predator::move(Cell& to, Cell& from)
{
  to.predator.push_back(from.predator.back());
  from.predator.clear();
  assert(to[0].ID==stupidModel.mapid(to.x,to.y));
  cellID=to[0].ID;
}


void Predator::hunt()
{
  Cell* myCell=getCell(cellID);
  //shuffle neighbourhood list (we must copy again, as per note in StupidBug::move()
  vector <objref*> neighbourlist(myCell->predatorNbrhd.size());
  int i=0;
  for (Ptrlist::iterator nbr=myCell->predatorNbrhd.begin(); nbr!=myCell->predatorNbrhd.end(); nbr++)
    neighbourlist[i++]=&*nbr;
  random_shuffle(neighbourlist.begin(),neighbourlist.end(),ran);

  vector<objref*>::iterator nbr;
  for (nbr=neighbourlist.begin(); nbr!=neighbourlist.end() && getCell(**nbr)->bug.size()==0; nbr++);
  if (nbr!=neighbourlist.end())
    {
      Cell* newCell=getCell(**nbr);
      stupidModel.killBug(newCell->bug[0]);
      if (newCell->predator.size()==0 && (*nbr)->proc==myid) // move only if destination is empty
	move(*newCell,*myCell);
   }
  else // move to first unoccupied location in shuffled list (or remain still)
    {
      for (nbr=neighbourlist.begin(); 
	   (*nbr)->ID!=cellID && getCell(**nbr)->predator.size()>0; nbr++);
      if ((*nbr)->ID!=cellID && (*nbr)->proc==myid)
	move(*getCell(**nbr),*myCell);
    }
}
      
void Predator::draw(const eco_string& canvas)
{
  tclcmd c;
  int scale=stupidModel.scale;
  c << canvas << "create rectangle" << (scale*x()) << (scale*y()) << 
    (scale*(x()+1)) << (scale*(y()+1)) << " -fill #0000ff -tags predators\n";
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
  parallel(args);
  gather();
  if (myid>0) return;
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete bugs\n";
  for (size_t i=0; i<bugs.size(); i++)
    bugs[i]->draw(canvas);
}

void StupidModel::drawPredators(TCL_args args)
{ 
  parallel(args);
  gather();
  if (myid>0) return;
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete predators\n";
  for (size_t i=0; i<predators.size(); i++)
    predators[i]->draw(canvas);
}

void StupidModel::drawCells(TCL_args args)
{
  parallel(args);
  gather();
  if (myid>0) return;
  eco_string canvas=args;
  tclcmd c;
  c << canvas << "delete all\n";
  for (omap::iterator i=objects.begin(); i!=objects.end(); i++)
    {
      Cell& cell=*getCell(*i);
      if (cell.predator.size())
	cell.predator[0]->draw(canvas);
      else if (cell.bug.size())
	cell.bug[0]->draw(canvas);
      else
	cell.draw(canvas);
    }
}

void StupidModel::grow(TCL_args args)
{
  parallel(args);
  assert(unique(bugs));
  for (Ptrlist::iterator i=begin(); i!=end(); i++)
    {
      Cell *cell=getCell(*i);
      cell->grow_food();
      if (cell->bug.size()>0)
	cell->bug[0]->grow();
    }
  assert(unique(bugs));
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

struct Prod_store
{
  int x,y;
  double p;
  Prod_store(int x,int y,double p): x(x), y(y), p(p) {}
};

void StupidModel::read_food_production(TCL_args args)
{
  parallel(args);
  char* filename=args;
  int moveDistance=args;
  int x, y, nx=0, ny=0;
  double prod;
  vector<Prod_store> prod_store;

  if (myid==0)
    {
      FILE *food_input=fopen(filename,"r");
      /* discard first 3 lines */
      for (int i=0; i<3; i++)
	for (int c=fgetc(food_input); c!='\n' && c!=EOF; c=fgetc(food_input));

      while (fscanf(food_input,"%d %d %lg",&x,&y,&prod)==3)
	{
	  nx=max(nx,x+1); ny=max(ny,y+1);
	  prod_store.push_back(Prod_store(x,y,prod));
	}
      
      fclose(food_input);
    }
  MPIbuf() << nx << ny << bcast(0) >> nx >> ny;
  Space::setup(nx,ny,moveDistance,false,0.1);
  gather();
  if (myid==0)
    for (vector<Prod_store>::iterator i=prod_store.begin(); i!=prod_store.end(); i++)
      getCell(*getObjAt(i->x,i->y))->food_production=i->p;
  Distribute_Objects();
}

void StupidModel::killBug(classdesc::ref<StupidBug>& bug)
{
  bugs.erase(find(bugs.begin(),bugs.end(),bug));
  bug->die();
}

void StupidModel::hunt(TCL_args args)
{
  parallel(args);
  for (size_t i=0; i<predators.size(); i++)
    predators[i]->hunt();
}
