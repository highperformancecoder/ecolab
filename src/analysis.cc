/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/


#include "ecolab.h"
#include <eco_hashmap.h>
#include "analysis.h"
#ifdef BLT
#include "analysisBLT.h"
#endif
#include <ctype.h>
#ifdef CAIRO
#include "plot.h"
#endif

#include <fstream>
#include <sstream>

using namespace ecolab;
using namespace std;
#include "arrays.h"
using array_ns::array;
using array_ns::pcoord;
#include "sparse_mat.h"
#include "ecolab_epilogue.h"

extern "C" void ecolab_analysis_link() {}

vector<string> palette_class::table;
int palette_class::size=0;



palette_class::palette_class()
{
  int  elemc;

  if (size==0)
   {
     tclvar palette("palette");
     CONST84 char **elem;

     if (exists(palette))
       {
	 if (Tcl_SplitList(interp(),palette,&elemc,&elem)!=TCL_OK) 
	   throw error("");
	 size=elemc;
	 table.resize(size);
	 for (int j=0; j<elemc; j++)
	     {
	       table[j] = elem[j];
	     }
	 Tcl_Free((char*)elem);
       }
     else
       {
         table.resize(1);
	 table[0]="black";
       }
   }
}

#ifdef HASH_hash_map
template <>
  struct hash<iarray*>
  {
    hash<int> h;
    size_t operator()(const iarray* x) const 
    {return h((int)x);}
  };
#elif !defined(HASH_TCL_hash)
namespace std
{
  struct less<iarray*>
  {
    bool operator() (const iarray* x, const iarray* y) const
    {return ((int)(x->list)) < ((int)(y->list));}
  };
}
#endif

/*
display cell

display a line plot of species densities as a function of time
if cell not specified, then cell 0 is assumed.
*/





template <class arr_t>
void display(std::string display, double tstep, 
             arr_t& density, const ecolab::array<int>& species=
             ecolab::array<int>())
{
  for (size_t i=0; i<display.length(); i++) if (!isalnum(display[i])) display[i]='_';
#if BLT
  tclcmd cmd;
  tclvar palette_var("palette");
  palette_class palette;
  //  static Blt_Vector time;
  static hash_map<arr_t*,double> lasttime;
  ecolab::array<int> offs;
  eco_strstream title, cmdstr;
  

  /* If first time, then create display widget in separate window */
  if (!lasttime.count(&density))
    {
      lasttime[&density]=0;
    }

  /* don't add unnecessary data */
  if (lasttime[&density]==tstep) return; 
  lasttime[&density]=tstep;

  std::string timename=display+"::time";
  cmd << timename<<"length\n"; 
  int timelength=atoi(cmd.result.c_str());
  for (int i=0; i<density.size(); i++)
      {
	eco_strstream elname;
	elname |display | "::line" | species[i];
	cmd << "array exists" << elname << '\n';
	if (!atoi(cmd.result.c_str()))  /* new species - create new graph element */
	  {
	    /* this statement no longer works within a namespace 
	       ([timename length] get byte compiled??) */
	    //	    cmd<< "vector" <<elname<< "(["<<timename<<" length])\n";
	    cmd<< "vector" <<elname;
	    cmd |"("| (tclcmd()<<timename<<"length\n").result |")\n"; 
	    /* ---- */
	    cmd|"."|display|".graph element create "|elname|
	      " -label \"\" -xdata "|timename|" -ydata "|elname;
	    if (exists(palette_var)) cmd << "-color" << palette[species[i]];
	    cmd << "-pixels 0\n";
	  }
	cmd | elname | " append " | density[i] | '\n';
      }

  cmd | timename |" append " | tstep | '\n';

#elif defined(CAIRO) // Cairo version here
  declare(plot, Plot, ("."+display+".plot").c_str());
  if (density.size() != species.size()) 
    throw error("size mismatch between density(%ld) and species(%ld)",
                long(density.size()),long(species.size()));
  for (size_t i=0; i<density.size(); ++i)
    plot.add(species[i], tstep, density[i]);
#endif
 
}  



NEWCMD(eco_display,3)
{ 
  /* 
     argv[1] = "tstep" - the independent variable
     argv[2] = "density" array to be plotted, and 
     argv[3] = "species" array used for colouring */
  double tstep=atof(argv[1]);
  declare( species, ecolab::array<int> , argv[3] );

  
  std::string displayN("display_"); displayN += argv[2];

  // declare( density, iarray, argv[2] );
  // density is either an array or iarray
  if (TCL_obj_properties().count(argv[2])==0)
    throw error("%s does not exist!",argv[2]);
  if (ecolab::array<int>* density = TCL_obj_properties()[argv[2]]->
      memberPtrCasted<ecolab::array<int> >())
    display(displayN,tstep,*density,species);
  else if (ecolab::array<double>* density = TCL_obj_properties()[argv[2]]->
           memberPtrCasted<ecolab::array<double> >())
    display(displayN,tstep,*density,species);
  else 
    throw error("%s is invalid type",argv[2]);
}

/* 
   connect_plot
Display a plot of the species' connectivities (i.e. the sparsity
structure of interaction matrix
*/

NEWCMD(eco_connect_plot,4)
{

  tclcmd cmd;
  int low_colour, high_colour;
  palette_class palette;

  /* 
     argv[1] = "interaction" - the sparse matrix
     argv[2] = "density" used for flagging extinctions 
     argv[3] = toplevel window of this widget
     argv[4] = scale
*/
  declare(interaction, sparse_mat, argv[1]);
  declare(density, ecolab::array<int>, argv[2]);
  const char *display=argv[3];
  double scale=atof(argv[4]);
  cmd|display|".graph delete all\n";
  ecolab::array<int> enum_clusters(interaction.diag.size());
  enum_clusters=1; 
  enum_clusters=enumerate(enum_clusters);
  for (unsigned i=0; i<interaction.row.size(); i++)
    { 
      if (enum_clusters[interaction.row[i]] > enum_clusters[interaction.col[i]])
	{
	  high_colour = enum_clusters[interaction.row[i]];
	  low_colour =  enum_clusters[interaction.col[i]];
	}
      else
	{
	  high_colour = enum_clusters[interaction.col[i]];
	  low_colour =  enum_clusters[interaction.row[i]];
	}
      enum_clusters = merge( enum_clusters==high_colour, 
			     low_colour, enum_clusters);
    }
      
  /* for grouping species into their ecologies */
  
  ecolab::array<int> map(interaction.diag.size()), mask;
  map=-1;
  
  for (int i=0; i<=max(enum_clusters); i++)
    {
      mask = enum_clusters==i;
      map = merge(  mask, enumerate(mask)+max(map)+1, map);
    }
      
  /* create coloured rectangles displaying the plot */
  for (unsigned i=0; i<interaction.row.size(); i++)
    {
      (cmd << display|".graph create rectangle") << 
	(map[interaction.col[i]]) * scale << 
	(map[interaction.row[i]]) * scale << 
	(map[interaction.col[i]]+1) * scale << 
	(map[interaction.row[i]]+1) * scale;
      //(int) shouldn't be necessary!
      if ((int)density[interaction.row[i]] == 0 ||   
	  (int) density[interaction.col[i]] ==0) 
	/* a species is extinct, connection is dead ! */
	cmd << " -fill wheat\n";
      else 
	cmd << " -fill " << palette[enum_clusters[interaction.row[i]]] 
	    << "\n";
    }
     
  /* do diagonals */
  for (size_t i=0; i<interaction.diag.size(); i++)
    (cmd <<  display|".graph create rectangle") << 
      (map[i])*scale << (map[i])*scale << 
      (map[i]+1)*scale << (map[i]+1)*scale <<  " -fill " << 
      (density[i]? palette[enum_clusters[i]]: "wheat") << "\n";
}

  Stats& Stats::operator<<=(float x)
  {
    sum+=x;
    sumsq+=x*x;
    max = std::max(max,x);
    min = std::min(min,x);
    array_ns::array<float>::operator<<=(x);
    return *this;
  }
  Stats& Stats::operator<<=(const array_ns::array<float>& x)
  {
    sum+=array_ns::sum(x);
    sumsq+=array_ns::sum(x*x);
    max = std::max(max,array_ns::max(x));
    min = std::min(min,array_ns::min(x));
    array_ns::array<float>::operator<<=(x);
    return *this;
  }


ecolab::array<double> HistoStats::histogram()
{
  ecolab::array<double> y(nbins,0);
  ecolab::array<float>& data=*this;
  if (logbins)
    {
      ecolab::array<float> pos_data=pack(data,data>0);
      if (!pos_data.size()) return y;
      logmin=log(array_ns::min(pos_data));
      float delta=(log(max)-logmin)/(nbins-0.1);
      float invdelta=1/delta;
      float invexpdelm1 = 1/(exp(delta)-1);
      ecolab::array<size_t> idx(invdelta*(log(pos_data)-logmin));
      y[idx]+=exp(-delta*idx)*invexpdelm1;
    }
  else
    {
      double invdelta=max>min? (nbins-0.1)/(max-min): 0;
      ecolab::array<size_t> idx(invdelta*(data-min));
      y[idx]+=1.0;
    }
  return y;
}

ecolab::array<double> HistoStats::bins()
{
  ecolab::array<double> x(nbins);
  ecolab::array<float>& data=*this;
  if (logbins && data.size())
    {
      logmin=log(array_ns::min(pack(data,data>0)));
      double delta=(log(max)-logmin)/(nbins-0.1);
      x=exp(pcoord(nbins)*delta+logmin);
    }
  else
    x=pcoord(nbins)*((max-min)/(nbins-0.1)) + min;
  return x;
}

//* Hill estimator 
double bestslope(double xmin, unsigned n, const float *x)
{
  double sumlog=0;
  for (unsigned i=0; i<n; i++) sumlog+=log(x[i]);
  return 1+n/(sumlog-n*log(xmin));
}

unsigned mini(double xmin, const ecolab::array<float>& x)
{
  // find location of first data > xmin
  unsigned mini;
  for (mini=0; mini<x.size() && x[mini]<xmin; mini++);
  return mini;
}

unsigned maxi(double xmax, const ecolab::array<float>& x)
{
  // find location of first data > xmin
  unsigned maxi;
  for (maxi=x.size(); maxi>0 && x[maxi-1]>xmax; maxi--);
  return maxi;
}

double bestslope(double xmin, const ecolab::array<float>& x)
{
  unsigned n=mini(xmin,x);
  return bestslope(xmin,x.size()-n,x.begin()+n);
}

// return maximum difference between cumulative distributions of
// power law model and data
double maxDiff(double xmin, const array_ns::array<float>& data)
{
  //assume data is sorted!
  unsigned n=mini(xmin,data);
  if (n==data.size()) return std::numeric_limits<double>::max();

  double maxDiff=0, slope=bestslope(xmin, data.size()-n, data.begin()+n);
  for (unsigned i=n; i<data.size(); i++)
    {
      assert (data[i]>=xmin);
      double diff=fabs( double(data.size()-i)/(data.size()-n) - pow(data[i]/xmin,-slope+1)  );
      if (diff>maxDiff) maxDiff=diff;
    }
  return maxDiff;
}

struct logdist
{
  virtual float operator()(float x) const=0;
};

double loglikelihood(const logdist& logp1, const logdist& logp2, unsigned n, float* x)
{
  double L=0, L2=0;
  for (unsigned i=0; i<n; i++)
    {
      double d=logp1(x[i])-logp2(x[i]); 
      L+=d;
      L2+=d*d;
    }
  double nsigma2=L2-L*L/n;
  return L/sqrt(2*nsigma2);
}

class PowerLaw: public logdist
{
  float slope, logslopem1onxmin, logxmin;
public:
  PowerLaw(float slope, float xmin): slope(slope),
                                     logslopem1onxmin(log((slope-1)/xmin)), 
                                     logxmin(log(xmin)) {}
  float operator()(float x) const {
    return logslopem1onxmin - slope * (log(x)-logxmin);}
};

class Exponential: public logdist
{
  float av, lognorm;
public:
  Exponential(float av, float xmin): av(av), lognorm(xmin/av-log(av)) {}
  float operator()(float x) const {return -x/av + lognorm;};
};

class Normal: public logdist
{
  float mu, inv2sigmasq, lognorm;
public:
  Normal(float mu, float sigma, float xmin): 
    mu(mu), inv2sigmasq(0.5/(sigma*sigma)), 
    lognorm(-log(1.2533141*sigma*erfc((xmin-mu)*0.70710678/sigma))) {}
  float operator()(float x) const {
    float xmmu=x-mu;
    return lognorm - xmmu*xmmu*inv2sigmasq;
  }
};

class LogNormal: public logdist
{
  float mu, inv2sigmasq, lognorm;
public:
  LogNormal(float mu, float sigma, float xmin): 
    mu(mu), inv2sigmasq(0.5/(sigma*sigma)),
    lognorm(-log(1.2533141*sigma*erfc((log(xmin)-mu)*0.70710678/sigma))) {} 
  float operator()(float x) const {
    float lnx=log(x), lnxmmu=lnx-mu;
    return lognorm - lnx - lnxmmu*lnxmmu*inv2sigmasq;
  }
};

array_ns::array<double> HistoStats::fitPowerLaw(TCL_args args)
{
  array_ns::array<double> ret(2);
  double& slope=ret[0], &xmin=ret[1];
  array<float>& data=*this;
  // we must have positive numbers here
  double m0=log(std::max(array_ns::min(data), std::numeric_limits<float>::min())); 
  double m1=log(std::max(array_ns::max(data), std::numeric_limits<float>::min()));
  sort(begin(),end());

  if (args.count==0)
    {
      // Do binary search on the interval x_min in [e^m0,e^m1] for min of maxDiff
      do
        {
          double left=maxDiff( exp((m1-m0)*0.25+m0), *this), 
            right=maxDiff( exp((m1-m0)*0.75+m0), *this);
          if (left < right)
            m1 -= (m1-m0)/2;
          else
            m0 += (m1-m0)/2;
        }
      while (m1-m0 > 0.01);
      xmin = exp((m1-m0)/2+m0);
    }
  else
    xmin=args;  //xmin specified by user

  unsigned start=mini(xmin,data);
  slope=bestslope(xmin,size()-start,begin()+start);
  return ret;
}

array_ns::array<double> HistoStats::fitLogNormal()
{
  array_ns::array<double> ret(2);
  double& mu=ret[0], &sigma=ret[1];
  Stats logstats; logstats <<= array<float>(log(static_cast<array<float>&>(*this)));
  mu=logstats.av(); sigma=logstats.stddev();

  return ret;
}

#include <memory>
using std::auto_ptr;

/* return a logdist object corresponding to the distribution string */ 
auto_ptr<logdist> distribution(char *distrname, float xmin)
{
  char *numstart=strchr(distrname,'(');
  if (numstart) 
    {
      *numstart++='\0'; //mark end of distribution key

      int nparms;
      float p1=strtof(numstart,&numstart), p2;
      if (*numstart==',')
        {
          p2=strtof(++numstart,NULL);
          nparms=2;
        }
      else
        nparms=1;

      if (strcmp(distrname,"powerlaw")==0 && nparms==2)
        return auto_ptr<logdist>(new PowerLaw(p1,xmin));
      else if (strcmp(distrname,"exponential")==0 && nparms==1)
        return auto_ptr<logdist>(new Exponential(p1,xmin));
      else if (strcmp(distrname,"normal")==0 && nparms==2)
        return auto_ptr<logdist>(new Normal(p1,p2,xmin));
      else if (strcmp(distrname,"lognormal")==0 && nparms==2)
        return auto_ptr<logdist>(new LogNormal(p1,p2,xmin));
    }
  //restore original string if needed
  if (numstart) distrname[strlen(distrname)]='('; 
  throw error("invalid distribution %s",distrname);
}
  
//return the log likelihood ratio between two distributions over a range xmin-xmax
//Convert to a p-value by applying fabs(erfc()) to the return value.
double HistoStats::loglikelihood(TCL_args args)
{
  float xmin=args[2];
  auto_ptr<logdist> p1=distribution(args,xmin), p2=distribution(args,xmin);
  sort(begin(),end());
  // third arg is minimum cutoff
  unsigned start=0;
  if (args.count) start=mini(args,*this);
  // fourth (optional) arg is maximum cutoff -- is this valid??
  unsigned n=size()-start;
  if (args.count) n=maxi(args,*this)-start;
  return ::loglikelihood( *p1, *p2, n, begin()+start);
  

}

TCLTYPE(Stats);
TCLTYPE(HistoStats);

NEWCMD(erfc,1) //used for exposing erfc to TCL
{
  tclreturn r;
  r<<erfc(atof(argv[1]));
}

#ifdef BLT
/* 

Some new histogram routines, storing temporary data in memory, rather
than a file. This should be OK, as most modern machines have multi MB
of RAM, and this is so much faster then disk I/O.

*/

TCLTYPE(histogram_data);

void histogram_data::init_BLT_vects(TCL_args args)
{
  name=(char*)args;
  if (Blt_GetVector(interp(),
                    const_cast<char*>((std::string("::")+name+"::x").c_str()),
                    &x)!=TCL_OK) 
    throw error(interp()->result);
  if (Blt_GetVector(interp(),
                    const_cast<char*>((std::string("::")+name+"::y").c_str()),
                    &y)!=TCL_OK) 
    throw error(interp()->result);
}  

void histogram_data::clear()
{
  data.clear();
  reread();
}

void histogram_data::reread()
{
  int i;
  if (Blt_ResizeVector(y,nbins)!=TCL_OK) 
    throw error(interp()->result);
  if (Blt_ResizeVector(x,nbins)!=TCL_OK) 
    throw error(interp()->result);
  array<double> hist=data.histogram();
  for (i=0; i<nbins; i++) y->valueArr[i]=hist[i];

  if (xlogison && data.size())
    {
      float log10=log(10.0);
      float delta=(log(max)-data.logmin)/(data.nbins-0.1);
      (tclcmd() |"."|name|".graph configure -barwidth") << 
	delta/log(10.0)<<"\n"; 
      (tclcmd() | "."|name|".graph xaxis configure -stepsize") << 
	delta*nbins/5.0/log(10.0)<<"\n"; 
      for (i=0; i<nbins; i++) 
	x->valueArr[i]=(i*delta+data.logmin)/log10;
    }
  else
    {
      double delta=(max-min)/(nbins-0.1);
      (tclcmd() | "."|name|".graph configure -barwidth") << 
	delta<<"\n"; 
      (tclcmd() | "."|name|".graph xaxis configure -stepsize") << 
	delta*nbins/5.0<<"\n"; 
      for (i=0; i<nbins; i++) 
	x->valueArr[i]=i*delta+min;
    }
}

void histogram_data::add_data(TCL_args args)
{

  double value=args;
  data<<=value;
  if (value<min) {min=value; reread();}
  if (value>max) {max=value; reread();}

  if (xlogison && value > 0)
    {
      float logmax=log(max);
      if (logmax>data.logmin)
        {
          double delta=(logmax-data.logmin)/(data.nbins-0.1);
          int idx=(log(value)-data.logmin)/delta;
          y->valueArr[idx]+=exp(-idx*delta)/(exp(delta)-1);
        }
    }
  else
    {
      double invdelta=max>min? (nbins-0.1)/(max-min): 0;
      y->valueArr[(size_t)((value-min)*invdelta)]+=1;
    }

  if (Blt_ResizeVector(y,nbins)!=TCL_OK) //refresh display 
    throw error(interp()->result);
}

void histogram_data::outputdat(TCL_args args)
{
  FILE *f=fopen(args,"w");
  double log10=log(10.0);
  assert(x->numValues==y->numValues);
  for (int i=0; i<x->numValues; i++)
    fprintf(f,"%g %g\n",xlogison? exp(log(10.0)*x->valueArr[i]): x->valueArr[i],
	    y->valueArr[i]);
  fclose(f);
}
#endif

namespace ecolab 
{

#ifdef CAIRO
  void HistoGram::outputdat(TCL_args args)
  {
    ofstream o((char*)args);
    array_ns::array<double> h=histogram(), b=bins();
    for (size_t i=0; i<h.size(); ++i)
      o<<b[i]<<" "<<h[i]<<endl;
  }

  TCLTYPE(HistoGram);
#endif


  namespace 
  {
    struct Item
    {
      float val;
      unsigned pos;
      bool operator<(const Item& x) const {return val<x.val || (val==x.val && pos<x.pos);}
      Item(float val, unsigned pos): val(val), pos(pos) {}
    };

    typedef vector<unsigned> Signature;

    template <class I>
    inline Signature signature(I& begin, const I& end, unsigned length)
    {
      set<Item> sortedItems;
      for (size_t i=0; i<length; ++i)
        if (begin!=end)
          sortedItems.insert(Item(*begin++, i));
        else
          sortedItems.insert(Item(0, i));
      vector<unsigned> r;
      r.reserve(length);
      for (set<Item>::iterator i=sortedItems.begin();
           i!=sortedItems.end(); ++i)
        r.push_back(i->pos);
      return r;
    }

    struct SmallsDiscretisedSeries: public vector<unsigned> 
    {
      typedef map<vector<Signature>, unsigned> NodeMap;
      NodeMap nodeMap;
      int numNodes;

      template <class I>
      SmallsDiscretisedSeries(I begin, const I& end, unsigned window):
        numNodes(0) {
        if (begin==end) return;
        
        vector<typename I::value_type::const_iterator> it, iend;
        for (; begin!=end; ++begin) {
          it.push_back(begin->begin());
          iend.push_back(begin->end());
        }

        while (it[0]!=iend[0])
          {
            vector<Signature> s;
            for (size_t i=0; i<it.size(); ++i) 
              if (it[i]!=iend[i])
                s.push_back(signature(it[i], iend[i], window));
            NodeMap::iterator i=nodeMap.find(s);
            if (i==nodeMap.end())
              i=nodeMap.insert(make_pair(s,numNodes++)).first;
            push_back(i->second);
          }
      }
      SmallsDiscretisedSeries(): numNodes(0) {}
    };
  }

  void NetworkFromTimeSeries::constructNet()
  {
    unsigned w=2;
    SmallsDiscretisedSeries last(begin(),end(),w);
    SmallsDiscretisedSeries beforeLast(last);
    SmallsDiscretisedSeries current;

    for (++w; w<begin()->size(); ++w)
      {
        current=SmallsDiscretisedSeries(begin(),end(),w);
        if (current.numNodes-last.numNodes < last.numNodes-beforeLast.numNodes)
          break;
        beforeLast=last;
        last=current;
      }
    
    // last is now the network we want to use
    map<pair<size_t,size_t>, int> transitionCount;
    SmallsDiscretisedSeries::iterator i=last.begin(), j=i;
    for (++i; i!=last.end(); ++i, ++j)
      transitionCount[make_pair(*j,*i)]++;
    
    net.clear(last.numNodes);
    for (map<pair<size_t,size_t>, int>::iterator i=transitionCount.begin();
         i!=transitionCount.end(); ++i)
      if (i->first.first!=i->first.second)
        net.push_back(Edge(i->first.first,i->first.second,i->second));
  }

  void NetworkFromTimeSeries::loadData(const char* filename)
  {
    ifstream f(filename);
    for (string buffer; getline(f,buffer);)
      {
        istringstream is(buffer);
        array<float> data;
        is>>data;
        if (columnIdx.size()!=size())
          for (size_t i=0; i<size(); ++i)
            (*this)[i]<<=data[i];
        else
          for (size_t i=0; i<size(); ++i)
            (*this)[i]<<=data[columnIdx[i]];
      }        
  }

  TCLTYPE(NetworkFromTimeSeries);

}



