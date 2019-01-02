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
#include <sstream>

#include <set>
#include "TCL_obj_base.h"

#include "TCL_obj_stl.h"

#include <vector>
#include <utility>

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
 
  template <class T> std::string quoteTCL(const T& x);


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

  template <class T>
  typename enable_if<Not<is_same<typename T::value_type,std::string> >,std::istream&>::T
  ContainerIn(std::istream& i, T& c)
  { 
    c.clear();
    typename T::value_type v;
    while (i>>v)
      {
#if defined(__cplusplus) && __cplusplus>=201103L
        c.push_back(std::move(v));
#else
        c.push_back(v);
#endif
      }
    return i;
  }
    
  template <class T>
  typename enable_if<is_same<typename T::value_type,std::string>,std::istream&>::T
  ContainerIn(std::istream& i, T& c)
  { 
    c.clear();
    /* input string may have spaces within elements, so could be quoted. We need to parse using Tcl_SplitList */
    char c1;
    std::string arg;
    while (i.get(c1)) arg+=c1;
    int  elemc;
    // exception safe resource handling
    struct CleanUp
    {
      CONST84 char **elem=NULL;
      ~CleanUp() {Tcl_Free((char*)(elem));}
    } e;
    if (Tcl_SplitList(interp(),arg.c_str(),&elemc,&e.elem)!=TCL_OK) 
      throw error("");
    typename T::value_type v;
    for (int j=0; j<elemc; ++j)
      c.push_back(e.elem[j]);
    return i;
  }
    

  template <class T, class CharT, class Traits>
  typename enable_if<is_container<T>, std::ostream&>::T
  operator<<(std::basic_ostream<CharT,Traits>& o, const T& v)
  {return ContainerOut(o,v);}

  template<class T>
  typename enable_if<is_container<T>, eco_strstream&>::T
  operator|(eco_strstream& s,const T& x) 
  {
    std::ostringstream tmp;
    ContainerOut(tmp,x);
    s<<tmp.str();
    return s;
  }


  template <class T, class CharT, class Traits>
  typename enable_if<is_sequence<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& i, T& v)
  {return ContainerIn(i,v);}

  // values need quoting if they contain spaces, and special chars need escaping
  template <class T>
  std::string quoteTCL(const T& x)
  {
    eco_strstream os;
    os | x;
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
    
  /// distinguish between maps and sets based on value_type of container
  template <class T> struct is_map: public false_type
  {
    static string keys() {return ".#members";}
    static string type() {return ".@is_set";}
  };

  struct is_map_map: public true_type
  {
    static string keys() {return ".#keys";}
    static string type() {return ".@is_map";}
  };


  template <class K, class V, class C, class A> struct is_map<std::map<K,V,C,A> >:
    public is_map_map {};

#if defined(__cplusplus) && __cplusplus>=201103L
  template <class K, class V, class C, class A> struct is_map<std::unordered_map<K,V,C,A> >:
    public is_map_map {};
#endif

  template <class T>
  typename enable_if<Not<is_map<T> >, typename T::value_type>::T
  readIn(std::istream& i)
  {typename T::value_type v; i>>v; return v;}

  template <class T>
  typename enable_if<is_map<T>,  typename T::value_type>::T
  readIn(std::istream& i)
  {
    typename remove_const<typename T::value_type::first_type>::type k;
    typename T::value_type::second_type v;
    i>>k>>v;
    return typename T::value_type(k,v);
  }
  
  template <class T, class CharT, class Traits>
  typename enable_if<is_associative_container<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& i, T& s)
  {
    s.clear();
    while (i)
      {
        typename T::value_type v=readIn<T>(i);
        if (i)
          s.insert(v);
      }
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

  template <class T> struct idx<const T>: public idx<T> {};


  /* support for extracting a list of keys in keyed data types (eg maps) */

  template <class T>
  typename enable_if<is_map<T>, void>::T
  keys_of(const T& o)
  { 
    tclreturn r;
    for (typename T::const_iterator i=o.begin(); i!=o.end(); i++)
      (r<<"\"")|i->first|"\"";
  }
  
  template <class T>
  typename enable_if<Not<is_map<T> >, void>::T
  keys_of(const T& o)
  { 
    tclreturn r;
    for (typename T::const_iterator i=o.begin(); i!=o.end(); i++)
      (r<<"\"")|*i|"\"";
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
      TCL_obj_of_vector_bool<const std::vector<bool>,idx_t>(o, d) {}
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

  // for distingushing between sets and maps with @elem functionality
  template <class T>
  typename enable_if<is_map<T>, TCL_obj_of<T,typename T::key_type>*>::T
  makeTCL_obj_of(T& o, const string& d) 
  {return new TCL_obj_of<T,typename T::key_type>(o,d);}

  template <class T>
  typename enable_if<Not<is_map<T> >, TCL_obj_of<T,iter>*>::T
  makeTCL_obj_of(T& o, const string& d) 
  {return new TCL_obj_of<T,iter>(o,d);}

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
  void TCL_obj_const_sequence(TCL_obj_t& targ, const string& desc, V& arg)
  {
    TCL_obj_register(targ,desc,arg);
    TCL_obj(targ,desc+".size",arg,&V::size);
    Tcl_CreateCommand(interp(),(desc+".@is_sequence").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<V,iter>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  // specialisation for vectors that exploits operator[]
  template <class V>
  void TCL_obj_const_vector(TCL_obj_t& targ, const string& desc, V& arg)
  {
    TCL_obj_register(targ,desc,arg);
    TCL_obj(targ,desc+".size",arg,&V::size);
    Tcl_CreateCommand(interp(),(desc+".@is_vector").c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)new TCL_obj_of<V,typename V::size_type>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  template <class V>
  void TCL_obj_sequence(TCL_obj_t& targ, const string& desc, V& arg)
  {
    ecolab::TCL_obj_const_sequence(targ,desc,arg);
    ecolab::ResizeFunctor<V>::createInTCL(arg,desc);
    TCL_obj(targ,desc+".clear",arg,&V::clear);
  }

  template <class T, class A>
  void TCL_obj_sequence(TCL_obj_t& targ, const string& desc, std::vector<T,A>& arg)
  {
    ecolab::TCL_obj_const_vector(targ,desc,arg);
    ecolab::ResizeFunctor<std::vector<T,A> >::createInTCL(arg,desc);
    TCL_obj(targ,desc+".clear",arg,&std::vector<T,A>::clear);
  }

  template <class T, class A>
  void TCL_obj_sequence(TCL_obj_t& targ, const string& desc, const std::vector<T,A>& arg)
  {
    ecolab::TCL_obj_const_vector(targ,desc,arg);
  }


  template <class VT>  struct KeyName: public std::string
  {KeyName(): std::string(".#members") {}};
  template <class K,class V>  struct KeyName<std::pair<K,V> >: public std::string
  {KeyName(): std::string(".#keys") {}};

  template <class T>
  void TCL_obj_associative_container(TCL_obj_t& targ, const string& desc, T& arg)
  {
    TCL_obj_register(targ,desc,arg);
    TCL_obj(targ,desc+".size",arg,&T::size);
    Tcl_CreateCommand(interp(),(desc+is_map<T>::type()).c_str(),
                      (Tcl_CmdProc*)null_proc,NULL,NULL);
    ClientData c=(ClientData)makeTCL_obj_of(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".@elem").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    c=(ClientData)makeTCL_obj_of(arg,desc);
    Tcl_CreateCommand(interp(),(desc+is_map<T>::keys()).c_str(),(Tcl_CmdProc*)keys,c,
                      (Tcl_CmdDeleteProc*)del_obj);
    c=(ClientData)new TCL_obj_of_count<T,typename T::key_type>(arg,desc);
    Tcl_CreateCommand(interp(),(desc+".count").c_str(),(Tcl_CmdProc*)elem,c,
                      (Tcl_CmdDeleteProc*)del_obj);
  }

  template <class T>
  typename enable_if<is_sequence<T>, void>::T
  TCL_objp(TCL_obj_t& t,const classdesc::string& desc, T& arg, dummy<1> d=0)
  {
    TCL_obj_sequence(t,desc,arg);
  }

  template <class T>
  typename enable_if<is_associative_container<T>, void>::T
  TCL_objp(TCL_obj_t& t,const classdesc::string& desc, T& arg, dummy<2> d=0)
  {
    TCL_obj_associative_container(t,desc,arg);
  }

}
using ecolab::TCL_obj;

namespace eco_strstream_ns
{
  using ecolab::operator<<;
}


#endif
