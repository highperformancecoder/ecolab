/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief serialisation for dynamic structures (graphs/trees and so on)
*/

#ifndef PACK_GRAPH_H
#define PACK_GRAPH_H

#include <vector>
#include <map>

#include <pack_base.h>
/*
  treenodes should really be deprecated in favour of graphnodes
*/

namespace classdesc
{
  ///serialise a tree (or DAG)
  template <class T>
  void pack(pack_t& targ, const string& desc, is_treenode dum, const T* const& arg)
  {
    int valid=0;
    if (arg==NULL) 
      pack(targ,desc,valid);
    else
      {
        valid=1;
        pack(targ,desc,valid);
        pack(targ,desc,*arg);
      }
  }

  ///unserialise a tree. 
  /** Serialised DAGs will be converted to a tree*/
  template <class T>
  void unpack(unpack_t& targ, const string& desc, is_treenode dum, T*& arg)
  {
    int valid;
    unpack(targ,desc,valid);
    if (valid) 
      {
        //      arg=new T;
        //      targ->alloced.push_back(arg);
        targ.alloced.push_back(new classdesc::PtrStore<T>);
        arg=static_cast<T*>(targ.alloced.back().data());
        unpack(targ,desc,*arg);
      }
  }

  /*
    pack up a graph (possibly containing cycles). 
    T must have the following members:
    V& operator*()         - dereference operator
    operator bool()        - returns true if valid, false otherwise
    T& operator=(const T&) - assignable x=y => &*x==&*y
    T()                    - construct an invalid object
    T(const T&)            - copyable

    Alloc<T>(T& x) - allocates a new object referenced by x

    V must be serialisable.
  */

  /** for non pointer types - eg smart pointers, an Alloc specialisation needs to be provided */
  template <class T> struct Alloc;
  template <class T> 
  struct Alloc<T*>
  {
    void operator()(pack_t& buf, T*& x) {
      buf.alloced.push_back(new PtrStore<T>);
      x=static_cast<T*>(buf.alloced.back().data());
    }
  };

  template <class T>
  inline void pack_graph(pack_t& buf, T& arg)
  {
    static std::map<T,int> graph_map;
    static std::vector<T*> restart;
    static unsigned recur_level=0;
    int nought=0;

    if (recur_level==0)
      pack(buf,string(),buf.recur_max);
    recur_level++;

    if (recur_level==buf.recur_max)
      restart.push_back(&arg); //save ref for restart
    else if (!arg)   //reference is invalid 
      pack(buf,string(),nought);
    else if (graph_map.count(arg)==0)
      {  
        int ID=graph_map.size()+1;
        graph_map[arg]=ID;
        pack(buf,string(),ID);
        pack(buf,string(),*arg);
      }
    else
      pack(buf,string(),graph_map[arg]);
    
    recur_level--;
    if (recur_level==0)   //process restarts
      {
        while (restart.size())
          {
            T& arg=*restart.back(); //we need to pop_back() before packing
            restart.pop_back();    //in case further restarts are needed
            pack_graph(buf,arg);
          }
        graph_map.clear();
      }
  }

  template <class T>
  void unpack_graph(pack_t& buf, T& arg)
  {
    static std::map<int,T> graph_map;
    static std::vector<T*> restart;
    static unsigned recur_level=0;
    int myid;
    Alloc<T> alloc;
    
    if (recur_level==0)
      unpack(buf,string(),buf.recur_max);
    recur_level++;

    if (recur_level==buf.recur_max)
      restart.push_back(&arg); //save ref for restart
    else
      {
        unpack(buf,string(),myid);
        if (myid==0) 
          arg=T();                //reset arg to invalid state
        else if (graph_map.count(myid))
          arg=graph_map[myid];
        else
          {
            alloc(buf,arg);
            graph_map[myid]=arg;
            unpack(buf,string(),*arg);
          }

      }
    recur_level--;
    if (recur_level==0)   //process restarts
      {
        while (restart.size())
          {
            T& arg=*restart.back(); //we need to pop_back() before packing
            restart.pop_back();    //in case further restarts are needed
            unpack_graph(buf,arg);
          }
        graph_map.clear();
      }

  }

  ///serialise a graph structure
  /** can contain cycles */

  template <class T>
  inline void pack(pack_t& targ, const string& desc,is_graphnode dum,const T& arg)
  {pack_graph(targ,arg);}

  ///unserialise a graph structure
  /** can contain cycles */
  template <class T>
  inline void unpack(pack_t& targ, const string& desc,is_graphnode dum,T& arg)
  {unpack_graph(targ,arg);}
  
}
#endif
