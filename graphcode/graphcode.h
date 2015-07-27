/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef GRAPHCODE_H
#define GRAPHCODE_H

/**
   select which map system (hmap or vmap) you wish to compile for 
*/
#ifndef MAP
#define MAP hmap
#endif

#undef x1str
#define x1str(x) #x
#undef xstr
#define xstr(x) x1str(x)
#undef cat
#define cat(x,y) x##y
#undef xcat
#define xcat(x,y) cat(x,y)

/// current namespace
#undef GRAPHCODE_NS
#define GRAPHCODE_NS xcat(graphcode_,MAP)

#ifdef MPI_SUPPORT
#include <classdescMP.h>

#ifdef PARMETIS
/* Metis stuff */
extern "C" void METIS_PartGraphRecursive
(unsigned* n,unsigned* xadj,unsigned* adjncy,unsigned* vwgt,unsigned* adjwgt,
 unsigned* wgtflag,unsigned* numflag,unsigned* nparts,unsigned* options, 
 unsigned* edgecut, unsigned* part);
extern "C" void METIS_PartGraphKway
(unsigned* n,unsigned* xadj,unsigned* adjncy,unsigned* vwgt,unsigned* adjwgt,
 unsigned* wgtflag,unsigned* numflag,unsigned* nparts,unsigned* options, 
 unsigned* edgecut, unsigned* part);

#include <parmetis.h>
#else
typedef int idxtype;  /* just for defining dummy weight functions */
#endif

#else
typedef int idxtype;  /* just for defining dummy weight functions */
#endif  /* MPI_SUPPORT */

#include <vector>
#include <map>
#include <algorithm>
#include <iostream>

#ifdef ECOLAB_LIB
#include "TCL_obj_base.h"
#endif

#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

#include <classdesc_access.h>
#include <object.h>
#include <pack_base.h>
#include <pack_stl.h>

namespace GRAPHCODE_NS
{
  /// Type used for Graph object identifier
  typedef unsigned long GraphID_t;
  /** a pin with ID==bad_ID cannot be inserted into a map or wire 
   - can be used for handling boundary conditions during graph construction */
  const GraphID_t bad_ID=~0UL;

  using std::vector;
  using std::map;
  using std::set;
  using std::find_if;
  using std::find;
  using std::cout;
  using std::endl;
  using namespace classdesc;
  using classdesc::string;

#ifdef MPI_SUPPORT
  /* MPI_Finalized only available in MPI-2 standard */
  inline bool MPI_running()
    {
      int fi, ff=0; MPI_Initialized(&fi); 
#if (defined(MPI_VERSION) && MPI_VERSION>1 || defined(MPICH_NAME))
      MPI_Finalized(&ff); 
#endif  /* MPI_VERSION>1 */
      return fi&&!ff;
    }
#endif  /* MPI_SUPPORT */

  /// return my processor no.
  inline  unsigned myid() 
    {
      int m=0;
#ifdef MPI_SUPPORT
      if (MPI_running()) MPI_Comm_rank(MPI_COMM_WORLD,&m);
#endif
      return m;
  }

  /// return number of processors
  inline unsigned nprocs() 
  {
    int m=1;
#ifdef MPI_SUPPORT
    if (MPI_running()) MPI_Comm_size(MPI_COMM_WORLD,&m);
#endif
    return m;
  }

  /** Utility for for returning \a val % \a limit 
      - \a val \f$\in[-l,2l)\f$ where \f$l\f$=\a limit
  */
  template <typename TYPE>
  inline TYPE Wrap(TYPE val, TYPE limit)
  {
    if( val >= limit )
      return val-limit;
    else if( val < 0 )
      return val+limit;
    else
      return val;
  }


  class object;
  class omap;
  class Graph;

  /* base object of graphcode - can be a pin or a wire - whatever */
  /** Reference to an object type */
  class objref
  {
    object *payload; /* referenced data */
    bool managed;
    friend class omap;
    CLASSDESC_ACCESS(objref);
  public:
    GraphID_t ID;  ///< object's ID
    unsigned int proc; ///< location of object

    struct id_eq /*predicate function testing whether x's ID is a given value*/
    {
      GraphID_t id; 
      id_eq(GraphID_t i): id(i) {} 
      bool operator()(const objref& x) {return x.ID==id;}
    };

    objref(GraphID_t i=0, int p=myid(), object *o=NULL): 
      payload(o), managed(false), ID(i), proc(p) {}
    objref(GraphID_t i, int p, object &o): 
      payload(&o), managed(false), ID(i), proc(p)  {}
    objref(const objref& x): payload(NULL) {*this=x;}
    ~objref() {nullify();}
    inline objref& operator=(const objref& x); 
    object& operator*()  {assert(payload!=NULL); return *payload;}
    object* operator->() {assert(payload!=NULL); return payload;}
    const object& operator*() const {assert(payload!=NULL); return *payload;}
    const object* operator->() const {assert(payload!=NULL); return payload;}
    /** add a reference to an object. 
        \a mflag indicates object should be deleted when the objref is
    */
    void addref(object* o, bool mflag=false) 
    {nullify(); payload=o; managed=mflag;}
    bool nullref() const {return payload==NULL;} /// is reference invalid?
    inline void nullify();  /// remove reference and delete target if managed
  };

#include xstr(MAP)

  /**
     Distributed objects database
  */
  class omap: public MAP
  {
    objref bad_thing;
  public:
    omap() {bad_thing.ID=bad_ID;}
    inline objref& operator[](GraphID_t i);
    inline omap& operator=(const omap& x);
  };

    
  inline omap& objectMap()  ///< Distributed objects database
  {
    static omap o;
    return o;
  }

  /**
     Vector of references to objects:
     - serialisable
     - semantically like vector<objref>, but with less storage
     - objects are not owned by this class
  */
  class Ptrlist 
  {
    vector<objref*> list;
    friend class omap;
    friend class Graph;
    friend void unpack(pack_t*,const string&,objref&);
  public:
    typedef vector<objref*>::size_type size_type;

    class iterator: public std::iterator<std::random_access_iterator_tag,objref>
    {
      typedef vector<objref*>::const_iterator vec_it;
      vec_it iter;
    public:
      iterator& operator=(const vec_it& x) {iter=x; return *this;}
      iterator() {}
      iterator(const iterator& x) {iter=x.iter;}
      iterator(vec_it x)  {iter=x;}
      objref& operator*() {return **iter;}
      objref* operator->() {return *iter;}
      iterator operator++() {return iterator(++iter);}
      iterator operator--() {return iterator(--iter);}
      iterator operator++(int) {return iterator(iter++);}
      iterator operator--(int) {return iterator(iter--);}
      bool operator==(const iterator& x) const {return x.iter==iter;}
      bool operator!=(const iterator& x) const {return x.iter!=iter;}
      size_t operator-(const iterator& x) const {return iter-x.iter;}
      iterator operator+(size_t x) const {return iterator(iter+x);}
    };
    iterator begin() const {return iterator(list.begin());}
    iterator end() const {return iterator(list.end());}
    objref& front()  {return *list.front();}
    objref& back()  {return *list.back();}
    const objref& front() const {return *list.front();}
    const objref& back() const {return *list.back();}
    objref& operator[](unsigned i) const 
    {
      assert(i<list.size());
      return *list[i];
    }
    void push_back(objref* x) 
    {
      if (x->ID!=bad_ID)
        list.push_back(&objectMap()[x->ID]); 
    }
    void push_back(objref& x) {push_back(&x);}     
    void erase(GraphID_t i) 
    {
      vector<objref*>::iterator it;
      for (it=list.begin(); it!=list.end(); it++) if ((*it)->ID==i) break;
      if ((*it)->ID==i) list.erase(it);
    }
    void clear() {list.clear();}
    size_type size() const {return list.size();}
    Ptrlist& operator=(const Ptrlist &x)
    {
      /* assignment of these refs must also fix up pointers to be consistent 
	 with the map */
      list.resize(x.size());
      for (size_type i=0; i<size(); i++)
        list[i]=&objectMap()[x[i].ID];
      return *this;
    }
    void lpack(pack_t& targ) const
    {
      targ<<size();
      for (iterator i=begin(); i!=end(); i++) targ << i->ID;
    }
    void lunpack(pack_t& targ) 
    {
      clear();
      size_type size; targ>>size;
      for (unsigned i=0; i<size; i++)
	{
	  GraphID_t id; targ>>id; 
	  push_back(&objectMap()[id]);
	}
    }
  };


  /** 
      base class for Graphcode objects.  an object, first and foremost
      is a \c Ptrlist of other objects it is connected to (maybe its
      neighbours, maybe its classes or families to which it belongs)
  */
  class object: public Ptrlist, virtual public classdesc::object
  {
  public:
#ifdef TCL_OBJ_BASE_H
    /// allow exposure to TCL
    virtual void TCL_obj(const classdesc::string& d) {}
#endif
    virtual ~object() {}
    virtual idxtype weight() const {return 1;} ///< node's weight (for partitioning)
    /// weight for edge connecting \c *this to \a x
    virtual idxtype edgeweight(const objref& x) const {return 1;} 
  };

  /*
    the #ifdef causes classdesc grief. 
  */

  inline void objref::nullify()  
  {
    if (managed) delete payload;
    managed=false; payload=NULL;
  } 

  inline objref& objref::operator=(const objref& x) 
  {
    nullify(); 
    ID=x.ID; proc=x.proc; 
    if (x.managed && x.payload)
      {
	payload=dynamic_cast<object*>(x->clone()); 
	managed=true;
      }
    else
      {
	payload=x.payload;
	managed=false;
      }
    return *this;
  }

  inline objref& omap::operator[](GraphID_t i)
  {
    if (i==bad_ID) 
      return bad_thing;
    else
      {
	objref& o=at(i);
	o.ID=i; /* enforce consistent ID field */
	return o;
      }
  } 

  inline omap& omap::operator=(const omap& x)
  {
    clear();
    for (const_iterator i=x.begin(); i!=x.end(); i++)
      {
	objref& o=at(i->ID);
	o.ID=i->ID; o.proc=i->proc;
	if (!i->nullref())
	  {
	    if (o.nullref() ||o->type()!=(*i)->type())
	    /* we need to create a new object */
              o.addref(dynamic_cast<object*>((*i)->clone()),true);
	  }
	else
	  o.nullify();
      }
    return *this;
  }

  /** Graph is a list of node refs stored on local processor, and has a
     map of object references (called objects) referring to the nodes.   */

  class Graph: public Ptrlist
  {
    //    CLASSDESC_ACCESS(class GRAPHCODE_NS::Graph);

  public: //(should be) private:
    vector<vector<GraphID_t> > rec_req; 
    vector<vector<GraphID_t> > requests; 
    unsigned tag;  /* tag used to ensure message groups do not overlap */
    bool type_registered(const object& x) {return x.type()>=0;}

  public:
    omap& objects;
    Graph(): tag(0), objects(objectMap()) {}
    Graph(Graph& g): objects(objectMap()) {*this=g;}

    Graph& operator=(const Graph& x)
    {
      static_cast<Ptrlist>(*this)=x; //Why is this needed?
      rec_req=x.rec_req; 
      requests=x.requests;
      tag=x.tag;
      rebuild_local_list();
      return *this;
    }

    /**
       Rebuild the list of locally hosted objects
    */
    void rebuild_local_list()
    {
      clear();
      for (omap::iterator p=objectMap().begin(); p!=objectMap().end(); p++)
	if (p->proc==myid()) Ptrlist::push_back(*p);
    }

    /**
       remove from local memory any objects not hosted locally
    */
    void clear_non_local()
    {
      for (omap::iterator i=objectMap().begin(); i!=objectMap().end(); i++) 
	if (i->proc!=myid()) i->nullify();
    }
    
    /** 
        print IDs of objects hosted on proc 0, for debugging purposes
    */
    void print(unsigned proc) 
    {
      if (proc==myid())
	for (iterator i=begin(); i!=end(); i++)
	  {
	    std::cout << " i->ID="<<i->ID<<":";
	    for (object::iterator j=(*i)->begin(); j!=(*i)->end(); j++)
	      std::cout << j->ID <<",";
	    std::cout << std::endl;
	  }
    }


    /* these method must be called on all processors simultaneously */
    void gather(); ///< gather all data onto processor 0
    /**
       Prepare cached copies of objects linked to by locally hosted objects
       - \a cache_requests=true means recompute the communication pattern
    */
    void Prepare_Neighbours(bool cache_requests=false);
    void Partition_Objects(); ///< partition
    /**
       distribute objects from proc 0 according to partitioning set in the 
       \c objref's \c proc field
    */
    inline void Distribute_Objects(); 

    /** 
        add the specified object into the Graph
    */
    objref& AddObject(object* o, GraphID_t id, bool managed=false) 
    {
      objref& p=objectMap()[id]; 
      o->type(); /* ensure type is registered */
      p.addref(o,managed); 
      assert(type_registered(*o));
      return p;
    }
    objref& AddObject(object& p, GraphID_t id) {return AddObject(&p,id);}
    /**
       add a new object of type T:
       - use as graph.AddObject<foo>(id);
    */
    template <class T>
    objref& AddObject(GraphID_t id) 
    {
      object* o=new T; 
      return AddObject(o,id,true);
    }
    /**
       add a new object initialised by \a master_copy
    */
    template <class T>
    objref& AddObject(const T& master_copy, GraphID_t id) 
    {
      object* o=new T(master_copy); 
      return AddObject(o,id,true);
    }

  };		   
  

  inline void 
  Graph::Distribute_Objects()
  {
#ifdef MPI_SUPPORT
    rec_req.clear();
    MPIbuf() << objectMap() << bcast(0) >> objectMap();
    rebuild_local_list();
#endif
  }

};

/* export pack/unpack routines */
//using GRAPHCODE_NS::pack;
//using GRAPHCODE_NS::unpack;

#ifdef _CLASSDESC
#pragma omit pack GRAPHCODE_NS::omap
#pragma omit unpack GRAPHCODE_NS::omap
#pragma omit isa GRAPHCODE_NS::omap
#pragma omit pack GRAPHCODE_NS::omap::iterator
#pragma omit unpack GRAPHCODE_NS::omap::iterator
#pragma omit isa GRAPHCODE_NS::omap::iterator
#pragma omit pack GRAPHCODE_NS::Ptrlist
#pragma omit unpack GRAPHCODE_NS::Ptrlist
#pragma omit pack GRAPHCODE_NS::Ptrlist::iterator
#pragma omit unpack GRAPHCODE_NS::Ptrlist::iterator
#pragma omit pack GRAPHCODE_NS::object
#pragma omit unpack GRAPHCODE_NS::object
#pragma omit pack GRAPHCODE_NS::objref
#pragma omit unpack GRAPHCODE_NS::objref
#pragma omit isa GRAPHCODE_NS::objref
#endif
 
namespace classdesc_access
{
  namespace cd=classdesc;

  template <>
  struct access_pack<GRAPHCODE_NS::object>
  {
    template <class U>
    void operator()(cd::pack_t& t,const cd::string& d, U& a)
    {pack(t,d,static_cast<const GRAPHCODE_NS::Ptrlist&>(a));}
  };

  template <>
  struct access_unpack<GRAPHCODE_NS::object>
  {
    template <class U>
    void operator()(cd::pack_t& t,const cd::string& d, U& a)
    {unpack(t,d,cd::base_cast<GRAPHCODE_NS::Ptrlist>::cast(a));}
  };

  template <>
  struct access_pack<GRAPHCODE_NS::omap>
  {
    template <class U>
    void operator()(cd::pack_t& buf, const cd::string& desc, U& arg)
    {
      buf << arg.size();
      for (typename U::iterator i=arg.begin(); i!=arg.end(); i++)
        buf << i->ID << *i;
    }
  };

  template <>
  struct access_unpack<GRAPHCODE_NS::omap>
  {
    template <class U>
    void operator()(cd::pack_t& buf, const cd::string& desc, U& arg)
    {
      typename U::size_type sz; buf>>sz;
      GRAPHCODE_NS::GraphID_t ID;
      for (; sz>0; sz--)
        {
          buf>>ID;
          buf>>arg[ID];
        }
    }
  };

  template <>
  struct access_pack<GRAPHCODE_NS::Ptrlist>
  {
    template <class U>
    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
    {arg.lpack(targ);}
  };

  template <>
  struct access_unpack<GRAPHCODE_NS::Ptrlist>
  {
    template <class U>
    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
    {arg.lunpack(targ);}
  };

#ifdef TCL_OBJ_BASE_H
  /* support for EcoLab TCL_obj method */
#ifdef _CLASSDESC
#pragma omit TCL_obj GRAPHCODE_NS::object
#pragma omit pack classdesc_access::access_TCL_obj
#pragma omit unpack classdesc_access::access_TCL_obj
#endif
  template <>
  struct access_TCL_obj<GRAPHCODE_NS::object>
  {
    template <class U>
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc,U& arg)
    {
      static bool not_in_virt=true; // possible thread safety problem
      if (not_in_virt)
        {
           TCL_obj(targ,desc+"",cd::base_cast<GRAPHCODE_NS::Ptrlist>::cast(arg));
           TCL_obj(targ,desc+".type",arg,&GRAPHCODE_NS::object::type);
           TCL_obj(targ,desc+".weight",arg,&GRAPHCODE_NS::object::weight);
           not_in_virt=false;
           arg.TCL_obj(desc); //This will probably recurse
           not_in_virt=true;
        }
    }
  }; 
#endif

  template <>
  struct access_pack<GRAPHCODE_NS::objref>
  {
    template <class U>
    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
    { 
      ::pack(targ,desc,arg.ID);
      ::pack(targ,desc,arg.proc);
      if (arg.nullref()) 
        targ<<-1;
      else 
       {
 	::pack(targ,desc,arg->type());
	arg->pack(targ);
       }
    }
  };

  template <>
  struct access_unpack<GRAPHCODE_NS::objref>
  {
    template <class U>
    void operator()(cd::pack_t& targ, const cd::string& desc, U& arg) 
    {
      ::unpack(targ,desc,arg.ID);
      ::unpack(targ,desc,arg.proc);
      int t; targ>>t;
      if (t<0) 
        {
	  arg.nullify();
	  return;
        }
      else if (arg.nullref() || arg->type()!=t)
        {
	  arg.nullify();
          cd::object* newobj=cd::object::create(t);
          GRAPHCODE_NS::object* obj=dynamic_cast<GRAPHCODE_NS::object*>(newobj);
          if (obj)
            arg.addref(obj,true);
          else
            {
              delete newobj; //invalid object heirarchy
              return;
            } 
        }
      arg->unpack(targ);
    }
  };
}
  
#undef str
#undef xstr

#endif  /* GRAPHCODE_H */
