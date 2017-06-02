#define USE_COMPOSITELESS_PHOTO_PUT_BLOCK
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
#include "stupid-field.h"
#include "stupid-field.cd"
#include "ecolab_epilogue.h"

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

/* system wide random number generator */
urand u;
make_model(u);

Space &space=stupidModel;

inline int ran(int n) {double r=u.rand(); return (r==1)? n-1: n*r;}

/* return ID for cell located at x,y */
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
      return bad_ID;  // out of bounds
  return x*ny+y;
}
 
void Space::setup(int nx_, int ny_, int moveDistance, bool toroidal_, double fp)
{
  nx=nx_, ny=ny_;
  assert(moveDistance <= nx/2 && moveDistance <= ny/2 );
  apron=moveDistance*ny;
  toroidal=toroidal_;
  offs=cellOffset(myid) - apron;
  size=nCells(myid);
  food_avail.resize(size+2*apron);
  food_production.resize(size+2*apron);
  for (size_t i=0; i<food_production.size(); i++)
    food_production[i]=fp;

  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      {
	objref& o=objects[mapid(i,j)];
	o.proc=(i*nprocs)/nx;
	if (o.proc==myid)
	  {
	    AddObject(Cell(),o.ID);
	    getCell(o)=Cell(i,j);
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
	    Cell& cell=getCell(o);
	    cell.bugNbrhd=cell;
	    // wire up predator neighbourhood - use Moore neighborhood, 
	    //even though specification is ambiguous
	    for (int ii=-1; ii<=1; ii++)
	      for (int jj=-1; jj<=1; jj++)
		cell.predatorNbrhd.push_back(objects[mapid(i+ii,j+jj)]);
	    assert(cell.size()==cell.bugNbrhd.size());
	  }
      }

  rebuild_local_list();
  Prepare_Neighbours(); // do this once here - just used for accessing food field
}

/* gather data onto processor 0 */
void Space::gather()
{
#ifdef MPI_SUPPORT
  Graph::gather();
  /* offsets and sizes for MPI_Gather */
  int offsets[nprocs], sizes[nprocs];
  for (size_t i=0; i<nprocs; i++)
    {
      offsets[i]=cellOffset(i);
      sizes[i]=nCells(i);
    }

  vector<double> recvbuf;
  if (myid==0) recvbuf.resize(nx*ny+2*apron);
  assert(size+apron<=food_avail.size());
  MPI_Gatherv(&food_avail[apron],size,MPI_DOUBLE,&recvbuf[apron],sizes,offsets,
	      MPI_DOUBLE,0,MPI_COMM_WORLD);
  if (myid==0)
    {
      swap(recvbuf,food_avail);
      recvbuf.resize(nx*ny+2*apron);
    }
  assert(size+apron<=food_production.size());
  MPI_Gatherv(&food_production[apron],size,MPI_DOUBLE,&recvbuf[apron],sizes,offsets,
	      MPI_DOUBLE,0,MPI_COMM_WORLD);
  if (myid==0)
    swap(recvbuf,food_production);
#endif
}

/* repartition data across processors after restart */
void Space::repartition()
{
  offs=cellOffset(myid) - apron;
  size=nCells(myid);
#ifdef MPI_SUPPORT
  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      objects[mapid(i,j)].proc=(i*nprocs)/nx;
  Distribute_Objects();

  if (myid==0)
    {
      //fill in global apron
      int N=nx*ny;
      for (int i=0; i<apron; i++)
        {
          food_avail[i]=food_avail[N+i];
          food_avail[N+apron+i]=food_avail[apron+i];
          food_production[i]=food_production[N+i];
          food_production[N+apron+i]=food_production[apron+i];
        }

      // send overlapped data to slave processors
      MPIbuf_array buf(nprocs);
      for (size_t i=1; i<nprocs; i++)
        {
          char* data=reinterpret_cast<char*>(&food_avail[cellOffset(i)]);
          size_t size=sizeof(double)*(nCells(i)+2*apron);
          buf[i].packraw(data,size);
          buf[i].isend(i);
          data=reinterpret_cast<char*>(&food_production[cellOffset(i)]);
          buf[i].packraw(data,size);
          buf[i].isend(i);
        }      
    }
  if (myid>0)
    {
      MPIbuf buf;
      char* data=reinterpret_cast<char*>(&food_avail[0]);
      buf.get();
      buf.unpackraw(data,buf.size());
      data=reinterpret_cast<char*>(&food_production[0]);
      buf.get();
      buf.unpackraw(data,buf.size());
    }      
#endif
}

#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic push
// ignore error that occurs when not building MPI version
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
inline void StupidBug::move()
{
  int newX, newY, nbr_idx;
  objref* newCellref=NULL;
  Cell &myCell=cell(),  *newCell=&myCell;
  double best_food=myCell.food_avail();

  // shuffle neighbourhood list, leaving the first element point to bug's
  // current cell
  //  random_shuffle(myCell->begin()+1,myCell->end());
  // the above did not work, because Ptrlist::iterator doesn't contain 
  // iterator_category (and other similar properties)
  //copy objref pointers into a temporary vector, then shuffle
  vector <objref*> neighbourlist(myCell.size());
  int i=0;
  for (Ptrlist::iterator nbr=myCell.bugNbrhd.begin(); 
       nbr!=myCell.bugNbrhd.end(); nbr++)
    neighbourlist[i++]=&*nbr;
  random_shuffle(neighbourlist.begin()+1,neighbourlist.end(),ran);

  // find an unoccupied cell in the neighbourhood
  for ( vector <objref*>::iterator nbr=neighbourlist.begin(); 
	nbr!=neighbourlist.end(); nbr++)
    {
      Cell& nCell=getCell(**nbr);
      if ( ((*nbr)->proc!=myid || !nCell.bug) && nCell.food_avail()>best_food)
        {
          newCell=&nCell;
          newCellref=*nbr;
          best_food=newCell->food_avail();
        }
    }
  if (newCell==&myCell)  //no better unoccupied neighbours
    return;  
#ifdef MPI_SUPPORT
  //preferential treatment to on-processor moves
  if (newCellref->proc!= myid)
    //request emmigration
    stupidModel.emmigration_list[newCellref->proc].
      push_back(pair<GraphID_t,GraphID_t>(cellID,newCellref->ID)); 
  else
#endif
    // swap myself to new cell
    moveTo(*newCell);
}
#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic pop
#endif

void StupidBug::grow()
{
  Cell &myCell=cell();
  double incr=min(max_consumption,myCell.food_avail()); 
  size+=incr;
  myCell.food_avail()-=incr;
  assert(myCell.bugNbrhd.size()==myCell.size());
}

/* return true if bug dies */
inline bool StupidBug::mortality()
{
  Cell &myCell=cell();
  if (size>repro_size)
    {
      for (int i=0; i<5; i++) // generate 5 new bugs
	for (int j=0; j<5; j++) // 5 goes at placement
	  {
	    objref& birth_ref=myCell.bugNbrhd[u.rand()*myCell.size()];
	    Cell& birth_loc=getCell(birth_ref);
	    if (!birth_loc.bug && birth_ref.proc==myid)
	      {
		bugArch.copyTo(birth_loc);
		stupidModel.bugs.push_back(birth_loc.bug);
		break;
	      }
	  }
      return true;
    }

  if (u.rand()>survivalProbability) return true;
  else return false;
}

void Predator::hunt()
{
  Cell& myCell=getCell(cellID);
  //shuffle neighbourhood list (we must copy again, as per note in StupidBug::move()
  vector <objref*> neighbourlist(myCell.predatorNbrhd.size());
  int i=0;
  for (Ptrlist::iterator nbr=myCell.predatorNbrhd.begin(); 
       nbr!=myCell.predatorNbrhd.end(); nbr++)
    neighbourlist[i++]=&*nbr;
  random_shuffle(neighbourlist.begin(),neighbourlist.end(),ran);

  vector<objref*>::iterator nbr;
  for (nbr=neighbourlist.begin(); 
       nbr!=neighbourlist.end() && !getCell(**nbr).bug; nbr++);
  if (nbr!=neighbourlist.end())
    {
      Cell& newCell=getCell(**nbr);
      stupidModel.killBug(newCell.bug);
      if (!newCell.predator && (*nbr)->proc==myid) // move only if destination is empty
	moveTo(newCell);
   }
  else // move to first unoccupied location in shuffled list (or remain still)
    {
      for (nbr=neighbourlist.begin(); 
	   (*nbr)->ID!=cellID && getCell(**nbr).predator; nbr++);
      if ((*nbr)->ID!=cellID && (*nbr)->proc==myid)
	moveTo(getCell(**nbr));
    }
}


void StupidModel::addBugs(TCL_args args)
{
  parallel(args);
  int  nBugs=args;
  objref *o;
  for (int i=0; i<nBugs; i+=nprocs)
    {
      do
	{
	  o=&objects[ran(objects.size())];
	} while (o->proc!=myid || getCell(*o).bug);
      Cell& cell=getCell(*o);
      bugArch.copyTo(cell); 
      /* initialise bug to a random size according to initBugDist */
      double sz=initBugDist->rand();
      cell.bug->size=(sz>0)? sz: 0;
      bugs.push_back(cell.bug);
    }
}

void StupidModel::addPredators(TCL_args args)
{
  using ::nprocs;
  using ::myid;  
  parallel(args);
  int  nPredators=args;
  objref *o;
  for (int i=0; i<nPredators; i++)
    {
      do
	{
	  o=&objects[ran(objects.size())];
	} while (o->proc!=myid || getCell(*o).predator);
      Cell& cell=getCell(*o);
      predatorArch.copyTo(cell); 
      predators.push_back(cell.predator);
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
          if (!getCell(dest).bug && !takenCells.count(dest))
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
              Cell& cell=getCell(emmigration_list[proc][i].first);
              migrants[proc]<<dest << *cell.bug;
              killBug(cell.bug);
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
	immigrant.copyTo(getCell(dest));
	bugs.push_back(getCell(dest).bug);
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

void StupidModel::grow(TCL_args args)
{
  parallel(args);
  grow_food();
  for (size_t i=0; i<bugs.size(); i++)
    bugs[i]->grow();
}

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
#ifdef MPI_SUPPORT
  MPIbuf() << nx << ny << prod_store << bcast(0) >> nx >> ny >> prod_store;
#endif
  Space::setup(nx,ny,moveDistance,false,0);
  GraphID_t id;
  for (vector<Prod_store>::iterator i=prod_store.begin(); i!=prod_store.end(); i++)
        if ((id=mapid(i->x,i->y))>=offs && 
            id-offs < food_production.size()) 
          food_production[id-offs]=i->p;
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

/*
  Visualisation support
*/

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
  c << "-tags {bugs animals}\n";
}

void Predator::draw(const eco_string& canvas)
{
  tclcmd c;
  int scale=stupidModel.scale;
  c << canvas << "create rectangle" << (scale*x()) << (scale*y()) << 
    (scale*(x()+1)) << (scale*(y()+1)) << " -fill #0000ff -tags {predators animals}\n";
}

void Space::draw(const eco_string& canvas)
{

  /*
    create a pixmap for displaying food
  */
  eco_string pixmapName=canvas+".food";
  Tk_PhotoHandle handle=Tk_FindPhoto(interp(),pixmapName.c_str());
  if (handle==NULL) //attach pixmap to canvas
    {
      tclcmd c;
      c << "image create photo"<<pixmapName<<
	"-width"<<nx*scale<<"-height"<<ny*scale<<"\n";
      c << canvas << "create image 0 0 -anchor nw -image"<<pixmapName<<"\n";
      handle=Tk_FindPhoto(interp(),pixmapName.c_str());
    }

  vector<unsigned char> pp(3*nx*ny);
  Tk_PhotoImageBlock block={&pp[0],nx,ny,nx*3,3,{0,1,2}};
  //  Tk_PhotoGetImage(handle,&block);

  for (int i=0; i<nx; i++)
    for (int j=0; j<ny; j++)
      {
	unsigned char *p=&pp[3*(i+j*nx)]; // start of pixel data
	GraphID_t id=mapid(i,j);
        //        if (objects[id].proc==myid)
          {
            Cell& cell=getCell(id);
            if (cell.predator) //display predator
              p[2]=255;    //blue
            else if (cell.bug) //display bug
              {
                p[0]=255; //red
                p[1]=     //green
                  p[2]=(cell.bug->size<10)? 255-int(cell.bug->size*25.6): 0; //blue
              }
            else
              { //display food levels
                double f=cell.food_avail();
                p[1]=f<0.5? int( f * 512.0 ): 255;
              }
          }
      }
  Tk_PhotoPutZoomedBlock(handle,&block,0,0,nx*scale,ny*scale,scale,scale,1,1);
}

void StupidModel::drawBugs(TCL_args args)
{
  parallel(args);
  Graph::gather();
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
  Graph::gather();
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
  eco_string canvas=args;
  Space::gather();
  if (myid==0)
    draw(canvas); 
}

/*
  Probe support (doesn't work in parallel)
  return a TCL object representing a bug 
  (if one exists at that location, and bug passed as third parameter) 
*/

eco_string StupidModel::probe(TCL_args args)
{
  int x=args, y=args;
  x/=scale; y/=scale; //reduce to application coordinates
  bool bug = strcmp(args,"bug")==0;
  eco_strstream id;
  static int cnt=0;
  Cell& cell=getCell(mapid(x,y));
  if (bug && cell.bug)
    {
      id | "bug" | cnt++;
      classdesc::ref<StupidBug> *bug=new classdesc::ref<StupidBug>(cell.bug); //make bug last eternally
      TCL_obj(null_TCL_obj,id.str(),**bug);
    }
  else
    {
      id | "cell" | cnt++;
      TCL_obj(null_TCL_obj,id.str(),cell);
    }
  return id.str();
}
