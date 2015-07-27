/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/*
  Standard C++ insists that function prototypes need to be declared
  before any template that might make use of them. Since these *might*
  include declarations from classdesc generated files, we put any such
  templates into a separate header file. Users will need to add an
  extra line 
     #include "TCL_obj_templates.h" 
  after including any .cd  files. 
*/

///\file

#ifndef TCL_OBJ_TEMPLATES_H
#define TCL_OBJ_TEMPLATES_H

#include "TCL_obj_base.h" // This should have been included already.

namespace
{
  int TCL_obj_template_not_included() {return 0;}
}

namespace ecolab
{
  /*
    This template ensures that an object is registered before processing members
  */
  template <class T>
  void TCL_obj(TCL_obj_t& t,const classdesc::string& desc, T& arg)
  {
    TCL_obj_register(desc,arg,t.member_entry_hook);
    classdesc_access::access_TCL_obj<T>()(t,desc,arg);
    TCL_obj_custom_register(desc,arg);
  }

  // deal with base classes
  template <class T>
  void TCL_obj_onbase(TCL_obj_t& t,const classdesc::string& desc, T& arg)
  {
    TCL_obj_register(desc,arg,t.member_entry_hook, true);
    classdesc_access::access_TCL_obj<T>()(t,desc,arg);
    TCL_obj_custom_register(desc,arg);
  }


  template <class T>
  struct TCL_obj_checkr: public TCL_obj_checkr_base
  {
    T* object;
    void pack(classdesc::pack_t& p) {::pack(p,string(),*object);}
    void unpack(classdesc::pack_t& p) {::unpack(p,string(),*object);}
  };


  template <class T>
  int TCL_obj_init(T& x)
  {
    if (isa(x,TCL_obj_t()))
      {
        TCL_obj_t *b=(TCL_obj_t *)(&x);
        b->check_functor=new TCL_obj_checkr<T>;
        ((TCL_obj_checkr<T>*)(b->check_functor))->object=&x;
      }
    return 1;
  }

  /// TCL register the object \a x with same name
#define make_model(x)                                                   \
  namespace x##_ns {                                                    \
    DEFINE_ECOLAB_LIBRARY;                                              \
    int TCL_obj_##x=(ecolab::TCL_obj_init(x),TCL_obj(ecolab::null_TCL_obj,(std::string)#x,x),1); \
  }

  /// TCL register the object \a x with same name
#define register(x) static int                                          \
  TCL_obj_register_##x=(ecolab::TCL_obj_register((string)#x,x,NULL),    \
                        TCL_obj(ecolab::null_TCL_obj,(string)#x,x),1)

    /// TCL register the type \a x 
#define TCLTYPE(x) TCLPOLYTYPE(x,x)
#define TCLPOLYTYPE(x,itf)                                              \
    namespace x_tcltype_##x {                                           \
      DEFINE_ECOLAB_LIBRARY;                                            \
                                                                        \
      int deleteobject(ClientData cd, Tcl_Interp *interp, int argc, const char **argv) \
      {                                                                 \
	assert( strcmp(argv[0]+strlen(argv[0])-strlen(".delete"),       \
		       ".delete")==0);                                  \
        std::string s(argv[0]);                                         \
        ecolab::TCL_obj_deregister(s.substr(0,s.length()-strlen(".delete"))); \
        return TCL_OK;                                                  \
      }                                                                 \
                                                                        \
      int createobject(ClientData cd, Tcl_Interp *interp, int argc, const char **argv) \
      {                                                                 \
	if (argc<2) throw ecolab::error("object name not specified");   \
	x *object=new x;                                                \
        ecolab::eraseAllNamesStartingWith(argv[1]);                    \
        TCL_obj(ecolab::null_TCL_obj,argv[1],*object);                  \
        assert(ecolab::TCL_newcommand((std::string(argv[1])+".delete").c_str())); \
	Tcl_CreateCommand(ecolab::interp(),                             \
                          (std::string(argv[1])+".delete").c_str(),     \
                          (Tcl_CmdProc*)deleteobject,                   \
			  (ClientData)object,NULL);                     \
        return TCL_OK;                                                  \
      }                                                                 \
                                                                        \
      int dummy=(                                                       \
                  /*gcc 3 bug */	/*assert(TCL_newcommand(#x)),*/ \
                 Tcl_CreateCommand                                      \
                 (ecolab::interp(),#x,                                  \
                  (Tcl_CmdProc*)createobject,NULL,NULL),1);             \
    }
    
    /* 
       Code for supporting arrays of things
    */


    template <class T>
    inline void tclret(const T&) {}

  inline void tclret(const char& x) {tclreturn() << x;}
  inline void tclret(const signed char& x) {tclreturn() << x;}
  inline void tclret(const unsigned char& x) {tclreturn() << x;}
  inline void tclret(const int& x) {tclreturn() << x;}
  inline void tclret(const unsigned int& x)  {tclreturn() << x;}
  inline void tclret(const long& x)  {tclreturn() << x;}
  inline void tclret(const unsigned long& x)  {tclreturn() << x;}
  inline void tclret(const short& x)  {tclreturn() << x;}
  inline void tclret(const unsigned short& x)  {tclreturn() << x;}
  inline void tclret(const bool& x)  {tclreturn() << x;}
  inline void tclret(const float& x)  {tclreturn() << x;}
  inline void tclret(const double& x)  {tclreturn() << x;}
  inline void tclret(const long double& x)  {tclreturn() << x;}
#if defined(__GNUC__)  
  inline void tclret(const long long& x)  {tclreturn() << x;}
  inline void tclret(const unsigned long long& x)  {tclreturn() << x;}
#endif

  template <class T>
  inline void asgtclret(T& x,TCL_args& y) {}

  //inline void asgtclret(char& x, TCL_args& y) {x=y; tclreturn() << x;}
  //inline void asgtclret(signed char& x, TCL_args& y) {x=y;tclreturn() << x;}
  //inline void asgtclret(unsigned char& x, TCL_args& y) {x=y;tclreturn() << x;}
  inline void asgtclret(int& x, TCL_args& y) {x=y;tclreturn() << x;}
  inline void asgtclret(unsigned int& x, TCL_args& y)  {x=y;tclreturn() << x;}
  inline void asgtclret(long& x, TCL_args& y)  {x=y;tclreturn() << x;}
  //inline void asgtclret(unsigned long& x, TCL_args& y)  {x=y;tclreturn() << x;}
  //inline void asgtclret(short& x, TCL_args& y)  {x=y;tclreturn() << x;}
  //inline void asgtclret(unsigned short& x, TCL_args& y)  {x=y;tclreturn() << x;}
  inline void asgtclret(bool& x, TCL_args& y)  {x=y;tclreturn() << x;}
  inline void asgtclret(float& x, TCL_args& y)  {x=y;tclreturn() << x;}
  inline void asgtclret(double& x, TCL_args& y)  {x=y;tclreturn() << x;}
  //inline void asgtclret(long double& x, TCL_args& y)  {x=y;tclreturn() << x;}
#if defined(__GNUC__)  
  //inline void asgtclret(long long& x, TCL_args& y)  {x=y;tclreturn() << x;}
  //inline void asgtclret(unsigned long long& x, TCL_args& y)  {x=y;tclreturn() << x;}
#endif

  template <class T>
  inline bool is_simpletype(const T& x) {return false;}

  inline bool is_simpletype(const char& x) {return true;}
  inline bool is_simpletype(const signed char& x) {return true;}
  inline bool is_simpletype(const unsigned char& x) {return true;}
  inline bool is_simpletype(const int& x) {return true;}
  inline bool is_simpletype(const unsigned int& x)  {return true;}
  inline bool is_simpletype(const long& x)  {return true;}
  inline bool is_simpletype(const unsigned long& x)  {return true;}
  inline bool is_simpletype(const short& x)  {return true;}
  inline bool is_simpletype(const unsigned short& x)  {return true;}
  inline bool is_simpletype(const bool& x)  {return true;}
  inline bool is_simpletype(const float& x)  {return true;}
  inline bool is_simpletype(const double& x)  {return true;}
  inline bool is_simpletype(const long double& x)  {return true;}
#if defined(__GNUC__)  
  inline bool is_simpletype(const long long& x)  {return true;}
  inline bool is_simpletype(const unsigned long long& x)  {return true;}
#endif

  template <class Object>
  struct array_handler
  {
    std::vector<int> dims;  /* dimensions of multidimensional array */
    Object *o;         /* pointer to array of Objects */
    void get(TCL_args a)
    {
      size_t i, idx;
      for (i=0, idx=0; i<dims.size(); i++)
        {
          idx*=dims[i];
          idx+=(int)a[i];
        };
      if (is_simpletype(o[idx]))
        tclret(o[idx]);
      else
        {
          /* create a default TCL name for index object */
          string name=a[-1]; 
          //	*strrchr((const char*)name,'.')='\0'; 
          name=name.substr(0,name.rfind('.'));/* strip off trailing .get from name */
          name+='('; 
          name+=(const char*)a[0];
          for (size_t i=1; i<dims.size(); i++)
            {
              name+=','; 
              name+=(const char*)a[i];
            }
          name+=')';
          TCL_obj(null_TCL_obj,name,o[idx]);
          tclreturn() << name;
        }
    }
    void set(TCL_args a)
    {
      size_t i, idx;
      for (i=0, idx=0; i<dims.size(); i++)
        {
          idx*=dims[i];
          idx+=(int)a;
        };
      if (is_simpletype(o[idx]) )
        asgtclret(o[idx],a);
      else
        {
          /* last argument is taken to be the TCL name of indexed object */
          string name=a;
          TCL_obj(null_TCL_obj,name,o[idx]);
          tclreturn() << name;
        }
    }
  };

  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_array ia, 
               T &arg,int dims,...)
  {
    int i, ncopies=1;
    va_list ap;
    va_start(ap,dims);

    /* initialise array handler object */
    array_handler<T> *ah=new array_handler<T>;
    ah->o=&arg;
    for (int i=0; i<dims; i++) ah->dims.push_back(va_arg(ap,int));

    /* create functors for the get and set methods */
    TCL_obj_functor<array_handler<T>,void>* get = 
      new TCL_obj_functor<array_handler<T>,void>;
    get->init(*ah,&array_handler<T>::get);
    get->name=desc+".get";
    TCL_OBJ_DBG(printf("registering %s\n",get->name.c_str()));
    Tcl_CreateObjCommand(interp(),get->name.c_str(),TCL_oproc,(ClientData)get,NULL);

    TCL_obj_functor<array_handler<T>,void>* set =
      new TCL_obj_functor<array_handler<T>,void>;
    set->init(*ah,&array_handler<T>::set);
    set->name=desc+".set";
    TCL_OBJ_DBG(printf("registering %s\n",set->name.c_str()));
    Tcl_CreateObjCommand(interp(),set->name.c_str(),TCL_oproc,(ClientData)set,NULL);
  }

  /* deal with treenode & graphnode pointers */
  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_treenode dum, T& arg)
  {if (arg) TCL_obj(targ,desc,*arg);}

  template <class T>
  void TCL_obj(TCL_obj_t& targ, const string& desc, classdesc::is_graphnode dum, T& arg)
  {if (arg) TCL_obj(targ,desc,*arg);}


  template<class T, class CharT, class Traits> 
  typename classdesc::enable_if<classdesc::Not<classdesc::is_enum<T> >, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y) 
  {
    throw error("operator>> not implemented for %s",typeid(T).name()); 
    return x;
  }

  template<class T, class CharT, class Traits> 
  typename classdesc::enable_if<classdesc::is_enum<T>, std::basic_istream<CharT,Traits>&>::T
  operator>>(std::basic_istream<CharT,Traits>& x,T& y) 
  {
    string s;
    x>>s;
    y=enum_keys<T>()(s);
    return x;
  }

  template <class T>
  std::istream& operator>>(std::istream& i, ecolab::ref<T>& a)
  {
    if (a) 
      i>>*a;
    else
      {T dummy; i>>dummy;} //throw away item
    return i;
  }
  
  template <class T>
  eco_strstream& operator|(eco_strstream& o, ecolab::ref<T>& a)
  {if (a) o|*a; return o;}

  template <class T> inline void member_entry<const T>::get() 
  {tclreturn()<<*memberptr;}
  template <class T> inline void member_entry<T>::get() 
  {tclreturn()<<*memberptr;}

  template <class T> inline void member_entry<T>::put(const char *s)
  {
#if (defined(__osf__))
    /* Tru 64 istream fails when eof encountered while reading
       stream !! - append a whitespace character to stream to
       ensure correct behaviour */
    const char *os=s;
    s=new char[strlen(s)+2]; strcpy((char*)s,(char*)os); 
    strcat((char*)s," ");
#endif
    assert(memberptr);
    std::istringstream r(s); r>>*memberptr;
    tclreturn() << s;
#if (defined(__osf__))
    delete [] s;
#endif
  }

  template <> inline void member_entry<string>::put(const char *s)
  {
    assert(memberptr);
    *memberptr=s;
    tclreturn() << s;
  }

  template <class T> inline void member_entry<const Enum_handle<T> >::get() 
  {tclreturn()<<string(*memberptr);}

  template <class T> inline void member_entry<const Enum_handle<T> >::put(const char *s)
  {
    assert(memberptr);
    *memberptr=s;
    tclreturn() << s;
  }

#if 0
  //template clash with std::string - resolve overload here
  std::istream& operator>>(std::istream& s, std::string& x)
  {
    return std::operator>>(s,x);
  }
#endif

  template <class T>
  void TCL_obj_ref<T>::pack(classdesc::pack_t& buf) {
    if (!datum) name.clear();
    ::pack(buf,"",name);
    if (name.length()) ::pack(buf,"",*datum);
  }
  template <class T>
  void TCL_obj_ref<T>::unpack(classdesc::pack_t& buf) {
    ::unpack(buf,"",name);
    if (name.length()) {
      set(name.c_str());
      ::unpack(buf,"",*datum);
    }
  }

  template <class T> 
  TCL_args& operator>>(TCL_args& a, T& x) 
  {std::istringstream s((char*)a); s>>x; return a;}

}

namespace classdesc_access
{
  template <class T>
  struct access_pack<ecolab::TCL_obj_ref<T> >
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& desc, 
                    ecolab::TCL_obj_ref<T>& arg)
    {arg.pack(buf);}
  };

  template <class T>
  struct access_unpack<ecolab::TCL_obj_ref<T> >
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& desc, 
                    ecolab::TCL_obj_ref<T>& arg)
    {arg.unpack(buf);}
  };

  template <class T>
  struct access_TCL_obj<classdesc::ref<T> >
  {
    void operator()(ecolab::TCL_obj_t& t, const classdesc::string& d, classdesc::ref<T>& a) 
    {
      if (a)
        TCL_obj(t,d,*a);
    }
  };

}

#endif /* TCL_OBJ_TEMPLATES_H */
