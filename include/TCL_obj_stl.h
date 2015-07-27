/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief TCL_obj support for STL containers
*/

#ifndef TCL_OBJ_STL_H
#define TCL_OBJ_STL_H
#include <string>
#include <iostream>

#include <set>
#include "TCL_obj_base.h"

#include "TCL_obj_stl.h"

//namespace eco_strstream_ns
//{
//  using ecolab::operator<<;
//}
#include <deque>
#include <vector>
#include <list>
#include <set>
#include <map>

namespace ecolab
{
  inline int null_proc() {return TCL_OK;}

  class TCL_obj_of_base
  {
  public:
    virtual ~TCL_obj_of_base(){}
    virtual string operator()(const char* index)=0;
    virtual void keys_of()=0;
  };

  static void del_obj(ClientData c)
  { delete (TCL_obj_of_base*)c;}
 
  // values need quoting if they contain spaces, and special chars need escaping
  template <class T>
  std::string quoteTCL(const T& x)
  {
    std::ostringstream os;
    os << x;
    std::string r;
    for (size_t i=0; i<os.str().length(); ++i)
      {
        if (strchr("{}\\\"", os.str()[i])!=NULL)
            r+='\\';
        r+=os.str()[i];
      }
    if (r.find(' ')!=std::string::npos)
      r="{"+r+"}";
    return r;
  }
    

  //  template <class T> void pushout(std::ostream& o, const T& c) {o<<c;}
  template <class F, class S> 
  std::ostream& operator<<(std::ostream& o, const std::pair<F,S>& p) 
  {return o<<quoteTCL(p.first)<<" "<<quoteTCL(p.second);}

  template <class T>
  std::ostream& ContainerOut(std::ostream& o, const T& c)
  { 
    for (typename T::const_iterator i=c.begin(); i!=c.end(); i++)
      {
        if (i!=c.begin()) o<<" ";
        o << quoteTCL(*i);
      }
    return o;
  }

  // TODO: handle spaces and active characters?
  template <class T>
  std::istream& ContainerIn(std::istream& i, T& c)
  { 
    c.clear();
    typename T::value_type v;
    while (i>>v) 
      c.push_back(v);
    return i;
  }
    
  template <class T, class A>
  std::ostream& operator<<(std::ostream& i, const std::vector<T,A>& v)
  {return ContainerOut(i,v);}

  template <class T, class A>
  std::istream& operator>>(std::istream& i, std::vector<T,A>& v)
  {return ContainerIn(i,v);}

  template <class T, class A>
  std::ostream& operator<<(std::ostream& i, const std::deque<T,A>& v)
  {return ContainerOut(i,v);}

  template <class T, class A>
  std::istream& operator>>(std::istream& i, std::deque<T,A>& v)
  {return ContainerIn(i,v);}
  template <class T, class A>
  std::ostream& operator<<(std::ostream& i, const std::list<T,A>& v)
  {return ContainerOut(i,v);}

  template <class T, class A>
  std::istream& operator>>(std::istream& i, std::list<T,A>& v)
  {return ContainerIn(i,v);}

  template <class T, class C, class A>
  std::ostream& operator<<(std::ostream& i, const std::set<T,C,A>& v)
  {return ContainerOut(i,v);}

  template <class K, class V, class C, class A>
  std::ostream& operator<<(std::ostream& i, const std::map<K,V,C,A>& v)
  {return ContainerOut(i,v);}

  template <class T, class C, class A>
  std::istream& operator>>(std::istream& i, std::set<T,C,A>& s)
  {
    s.clear();
    T v;
    while (i>>v) 
      s.insert(v);
    return i;
  }

  template <class K, class V, class C, class A>
  std::istream& operator>>(std::istream& i, std::map<K,V,C,A>& m)
  {
    m.clear();
    K k; V v;
    while (i>>k>>v) 
      m.insert(typename std::map<K,V,C,A>::value_type(k,v));
    return i;
  }

  static  int elem(ClientData v, Tcl_Interp *interp, int argc, 
                   CONST84 char** argv) 
  {
    tclreturn r;
    if (argc<2) 
      {
        r << "insufficient arguments";
        return TCL_ERROR;
      }
    r << (*(TCL_obj_of_base*)v)(argv[1]);
    return TCL_OK;
  }

  static int keys(ClientData v, Tcl_Interp *interp, int argc, 
                  CONST84 char** argv) 
  {((TCL_obj_of_base*)v)->keys_of(); return TCL_OK;}

  template <class T> struct idx
  {T operator()(const char *x) {throw error("invalid index type");}};

  template <> struct idx<int>
  {int operator()(const char *x){return atoi(x);}};

  template <> struct idx<unsigned int>
  {int operator()(const char *x){return atoi(x);}};

  template <> struct idx<long>
  {long operator()(const char *x){return atol(x);}};

  template <> struct idx<unsigned long>
  {long operator()(const char *x){return atol(x);}};

  template <> struct idx<const char *>
  {const char* operator()(const char *x){return x;}};

  template <> struct idx<std::string>
  {std::string operator()(const char *x){return std::string(x);}};

  /* support for extracting a list of keys in keyed data types (eg maps) */

  template <class T>
  inline void keys_of(const T& o) {}

  template <class K, class C, class A>
  inline void keys_of(const std::set<K,C,A>& o)
  { 
    tclreturn r;
    for (typename std::set<K,C,A>::const_iterator i=o.begin(); i!=o.end(); i++)
      r<<*i;
  }  

  template <class K, class T, class C, class A>
  inline void keys_of(std::map<K,T,C,A>& o)
  { 
    tclreturn r;
    for (typename std::map<K,T,C,A>::const_iterator i=o.begin(); i!=o.end(); i++)
      r<<i->first;
  }

  //handle vector<bool> as a special case
  template <> struct member_entry<std::vector<bool>::reference>: public member_entry_base
  {
    typedef std::vector<bool>::reference R;
    std::vector<bool> defaultDummy;
    R memberptr;
    member_entry(): defaultDummy(1), memberptr(defaultDummy[0]) {}
    member_entry(R& x): memberptr(x) {}
    inline void get() {tclreturn() << (bool)memberptr;}
    inline void put(const char *s) {tclreturn() << s; memberptr=atoi(s);} 
  };

  template <class T, class idx_t>
  class TCL_obj_of: TCL_obj_of_base
  {
    T& obj;
    string desc;
  public:
    TCL_obj_of(T& o, const string& d): obj(o), desc(d) {}
    string operator()(const char* index) 
    {
      string elname=desc+"("+index+")";
      // because TCL_obj_register assumes second time around calls are
      // base class registrations, we need to erase the member_entry
      // before registering the new handler, in case the element's
      // address has changed due to container resizing or whatever
      TCL_obj_properties().erase(elname); 
      TCL_obj(ecolab::null_TCL_obj,elname,obj[idx<idx_t>()(index)]);
      return elname;
    }
    void keys_of() {ecolab::keys_of(obj);}
  };

  template <class T, class idx_t>
  class TCL_obj_of_vector_bool: TCL_obj_of_base
  {
    T& obj;
    string desc;
  public:
    TCL_obj_of_vector_bool(T& o, const string& d): obj(o), desc(d) {}
    string operator()(const char* index) 
    {
      string elname=desc+"("+index+")";
      TCL_obj_properties().erase(elname); 
      typename T::reference r(obj[idx<idx_t>()(index)]);
      TCL_obj(ecolab::null_TCL_obj,elname,r);
      return elname;
    }
    void keys_of() {ecolab::keys_of(obj);}
  };

  template <class idx_t>
  struct TCL_obj_of<std::vector<bool>,idx_t>: 
    public TCL_obj_of_vector_bool<std::vector<bool>,idx_t> 
  {
    TCL_obj_of(std::vector<bool>& o, const string& d): 
      TCL_obj_of_vector_bool<std::vector<bool>,idx_t>(o, d) {}
  };
  
  template <class idx_t>
  struct TCL_obj_of<const std::vector<bool>,idx_t>: 
    public TCL_obj_of_vector_bool<const std::vector<bool>,idx_t> 
  {
    TCL_obj_of(const std::vector<bool>& o, const string& d): 
      TCL_obj_of_vector_bool<std::vector<bool>,idx_t>(o, d) {}
  };

  /* special case when only forward iterators are available */
  class iter {};
  template <class T>
  class TCL_obj_of<T,iter>: TCL_obj_of_base
  {
    T& obj;
    string desc;
  public:
    TCL_obj_of(T& o, const string& d): obj(o), desc(d) {}
    string operator()(const char* index) 
    {
      string elname=desc+"("+index+")";
      TCL_obj_properties().erase(elname); 
      typename T::iterator j;
      int i;
      for (i=0, j=obj.begin(); i<atoi(index); i++, j++);
      TCL_obj(ecolab::null_TCL_obj,elname,*j);
      return elname;
    }
    void keys_of() {ecolab::keys_of(obj);}
  };

  /* special case to handle count method */
  template <class T, class idx_t>
  class TCL_obj_of_count: TCL_obj_of_base
  {
    const T& obj;
    string desc;
  public:
    TCL_obj_of_count(const T& o, const string& d): obj(o), desc(d) {}
    string operator()(const char* index) 
    {return (eco_strstream() << obj.count(idx<idx_t>()(index))).str();}
    void keys_of() {ecolab::keys_of(obj);}
  };

  // specialised functor for calling resize, as it may be overloaded,
  // or may have a default second argument
  template <class S> struct ResizeFunctor: public cmd_data
  {
    S& x;
    ResizeFunctor(S& x): x(x) {}
    void proc(int argc, Tcl_Obj *const argv[]) {
      x.resize((size_t)TCL_args(argc, argv));
    }
    static void createInTCL(S& x, const string& d) {
      Tcl_CreateObjCommand(interp(),(d+".resize").c_str(), TCL_oproc,
                           (ClientData)new ResizeFunctor<S>(x),
                           TCL_cmd_data_delete);
    }
    void proc(int, const char **) {}  
  };

  template <class V>
  void TCL_obj_vector(TCL_obj_t& targ, const string& desc, V& arg)
  {
    TCL_obj_register(desc,arg,targ.member_entry_hook);
    TCL_obj(targ,desc+".size",arg,&V::size);
    Tcl_CreateCommand(interp(),(desc+".@is_vector").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<V,int>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  template <class T>
  void TCL_obj_deque(TCL_obj_t& targ, const string& desc, T& arg)
  {
    TCL_obj_register(desc,arg,targ.member_entry_hook);
    TCL_obj(targ,desc+".size",arg,&T::size);
    Tcl_CreateCommand(interp(),(desc+".@is_deque").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<T,int>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  template <class T>
  void TCL_obj_list(TCL_obj_t& targ, const string& desc, T& arg)
  {
    TCL_obj_register(desc,arg,targ.member_entry_hook);
    TCL_obj(targ,desc+".size",arg,&T::size);
    Tcl_CreateCommand(interp(),(desc+".@is_list").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<T,iter>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  template <class T>
  void TCL_obj_set(TCL_obj_t& targ, const string& desc, T& arg)
  {
    TCL_obj_register(desc,arg,targ.member_entry_hook);
    TCL_obj(targ,desc+".size",arg,&T::size);
    Tcl_CreateCommand(interp(),(desc+".@is_set").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<T,iter>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    c=(ClientData)new TCL_obj_of_count<T,typename T::value_type>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".#members").c_str(),(Tcl_CmdProc*)keys,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    Tcl_CreateCommand(interp(),(desc+".count").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  template <class T>
  void TCL_obj_map(TCL_obj_t& targ, const string& desc, T& arg)
  {
    TCL_obj(targ,(desc+".size").c_str(),arg,&T::size);
    Tcl_CreateCommand(interp(),(desc+".@is_map").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<T,typename T::key_type>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    Tcl_CreateCommand(interp(),(desc+".#keys").c_str(),(Tcl_CmdProc*)keys,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    c=(ClientData)new TCL_obj_of_count<T,typename T::key_type>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".count").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

}
using ecolab::TCL_obj;

namespace classdesc_access
{
  namespace cd=classdesc;
  template <class T, class A> struct access_TCL_obj<std::vector<T,A> >
  {
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, std::vector<T,A>& arg)
    {
      ecolab::TCL_obj_vector(targ,desc,arg);
      ecolab::ResizeFunctor<std::vector<T,A> >::createInTCL(arg,desc);
      TCL_obj(targ,desc+".clear",arg,&std::vector<T,A>::clear);
    }
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, const std::vector<T,A>& arg)
    {
      ecolab::TCL_obj_vector(targ,desc,arg);
    }
  };

  template <class T, class A> struct access_TCL_obj<std::deque<T,A> >
  {
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, std::deque<T,A>& arg)
    {
      ecolab::TCL_obj_deque(targ,desc,arg);
      ecolab::ResizeFunctor<std::deque<T,A> >::createInTCL(arg,desc);
      TCL_obj(targ,desc+".clear",arg,&std::deque<T,A>::clear);
    }
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, const std::deque<T,A>& arg)
    {
      ecolab::TCL_obj_deque(targ,desc,arg);
    }
  };

  template <class T, class A> struct access_TCL_obj<std::list<T,A> >
  {
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, std::list<T,A>& arg)
    {
      ecolab::TCL_obj_list(targ,desc,arg);
      ecolab::ResizeFunctor<std::list<T,A> >::createInTCL(arg,desc);
      TCL_obj(targ,desc+".clear",arg,&std::list<T,A>::clear);
    }
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, const std::list<T,A>& arg)
    {ecolab::TCL_obj_list(targ,desc,arg);}
  };

  template <class T, class C, class A> struct access_TCL_obj<std::set<T,C,A> >
  {
    template <class U>
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, U& arg)
    {ecolab::TCL_obj_set(targ,desc,arg);}
  };

  template <class K, class T, class C, class A> 
  struct access_TCL_obj<std::map<K,T,C,A> >
  {
    template <class U>
    void operator()(cd::TCL_obj_t& targ, const cd::string& desc, U& arg)
    {ecolab::TCL_obj_map(targ,desc,arg);}
  };
}


#endif
