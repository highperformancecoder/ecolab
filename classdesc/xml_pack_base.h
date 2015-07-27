/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief XML serialisation descriptor
*/

#ifndef XML_PACK_BASE_H
#define XML_PACK_BASE_H
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <assert.h>

#include <classdesc.h>
#include <xml_common.h>
#include <stdexcept>

namespace classdesc
{
  inline std::string xml_quote(char c)
  {
    switch (c)
      {
      case '&': return "&amp;";
      case '<': return "&lt;";
      case '>': return "&gt;";
      case '\'': return "&apos;";
      case '"': return "&quot;";
      }
    if (!isgraph(c))
      {
        std::ostringstream s;
        s<<"&#"<<int(c)<<";";
        return s.str();
      }
    return std::string(1,c);
  }

  /**
     XML serialisation object
  */
  class xml_pack_t
  {
    std::ostream* o; // weak reference, allows for assignability 
    int taglevel;
    // count number of ids separated by '.'s in a string
    int level(const string& xx) {
      const char* x=xx.c_str();
      int l;
      if (*x=='\0') return 0;
      for (l=1; *x!='\0'; x++) if (*x=='.') l++;
      return l;
    }

    void pretty(const string& d) {if (prettyPrint) *o << std::setw(level(d)) <<"";}
    void endpretty() {if (prettyPrint) *o<<std::endl;}

    /**
       emit a tag \a d if current nesting level allows
       return true if tag created
    */
    bool tag(const string& d) {  
      int l=level(d);
      bool ret = taglevel < l; //return true if tag created
      if (ret)
        {
          pretty(d);
          *o<<"<"<<tail(d);
          if (l==1 && !schema.empty())
            *o<<" xmlns=\""<<schema<<"\"";
          *o<<">";
          endpretty();
          taglevel=l;
        }
      assert(taglevel==level(d));
      return ret;
    }
    /** emit end tag */
    void endtag(const string& d) {
      taglevel--;
      pretty(d);
      *o<<"</"<<tail(d)<<">";
      endpretty();
    }

    friend class Tag;
  public:
    string schema; 
    bool prettyPrint; /// if true, the layout XML in more human friendly form

    xml_pack_t(std::ostream& o, const string& schema=""): 
      o(&o), taglevel(0), schema(schema), prettyPrint(false) {}

    class Tag  ///<utility structure for handling tag/endtag
    {
      xml_pack_t* t;
      string d;
    public:
      Tag(xml_pack_t& t, const string& d): t(t.tag(d)? &t: 0), d(d) {}
      ~Tag() {if (t) t->endtag(d);}
    };

    /**
       pack simple type
    */
    template <class T>
    void pack(const string& d, const T&x)
    {
      std::string tag=tail(d);
      pretty(d);
      *o << "<"<<tag<<">" << x << "</"<<tag<<">";
      endpretty();
    }
    /**
       pack an untagged simple type
    */
    template <class T>
    void pack_notag(const string& d, const T&x) 
    {/*pretty(d);*/ o<<x; /*endpretty();*/}

  };

  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  xml_packp(xml_pack_t& x,const string& d,T& a) 
  {x.pack(d,a);}

  template <> inline void xml_packp(xml_pack_t& x,const string& d, bool& a)
  {x.pack(d, a? "true": "false");}

  /**
     handle enums
  */

  template <class T> 
  typename enable_if<is_enum<T>, void>::T
  xml_packp(xml_pack_t& x, const string& d, T& arg)
  {x.pack(d,string(Enum_handle<T>(arg)));}

}

namespace classdesc_access
{
  template <class T> struct access_xml_pack;
}

template <class T> void xml_pack(classdesc::xml_pack_t&,const classdesc::string&, const T&);

template <class T> void xml_pack(classdesc::xml_pack_t&,const classdesc::string&, T&);

template <class T> classdesc::xml_pack_t& operator<<(classdesc::xml_pack_t& t, const T& a);

inline void xml_pack(classdesc::xml_pack_t& x,const classdesc::string& d, 
                     std::string& a) 
{
  std::string tmp;
  for (std::string::size_type i=0; i<a.length(); i++) tmp+=classdesc::xml_quote(a[i]);
  x.pack(d,tmp);
}

inline void xml_pack(classdesc::xml_pack_t& x,const classdesc::string& d, const std::string& a) 
{xml_pack(x,d,const_cast<std::string&>(a));}

/* now define the array version  */
#include <stdarg.h>

  template <class T> void xml_pack(classdesc::xml_pack_t& x,const classdesc::string& d,classdesc::is_array ia,
                                     T& a, int dims,size_t ncopies,...) 
  {
    va_list ap;
    va_start(ap,ncopies);
    for (int i=1; i<dims; i++) ncopies*=va_arg(ap,int); //assume that 2 and higher D arrays dimensions are int
    va_end(ap);
    classdesc::xml_pack_t::Tag tag(x,d);

    // element name is given by the type name
    classdesc::string eName=classdesc::typeName<T>().c_str();
    // strip leading namespace and qualifiers
    const char *e=eName.c_str()+eName.length();
    while (e!=eName.c_str() && *(e-1)!=' ' && *(e-1)!=':') e--;

    for (size_t i=0; i<ncopies; i++) xml_pack(x,d+"."+e,(&a)[i]);
  }

template <class T1, class T2>
void xml_pack(classdesc::xml_pack_t& x, const classdesc::string& d, 
              const std::pair<T1,T2>& arg)
{
  classdesc::xml_pack_t::Tag t(x,d);
  xml_pack(x,d+".first",arg.first);
  xml_pack(x,d+".second",arg.second);
}  

namespace classdesc
{
  template <class T> typename
  enable_if<Or<is_sequence<T>, is_associative_container<T> >, void>::T
  xml_packp(xml_pack_t& x, const string& d, T& arg, dummy<1> dum=0)
  {
    xml_pack_t::Tag tag(x,d); 
    // element name is given by the type name
    string eName=typeName<typename T::value_type>().c_str();
    eName=eName.substr(0,eName.find('<')); //trim off any template args
    // strip leading namespace and qualifiers
    const char *e=eName.c_str()+eName.length();
    while (e!=eName.c_str() && *(e-1)!=' ' && *(e-1)!=':') e--;

    for (typename T::const_iterator i=arg.begin(); i!=arg.end(); ++i)
      ::xml_pack(x,d+"."+e,*i);
  }

  template <class T>
  void xml_pack_onbase(xml_pack_t& x,const string& d,T& a)
  {::xml_pack(x,d+basename<T>(),a);}

}

using classdesc::xml_pack_onbase;

/* member functions */
template<class C, class T>
void xml_pack(classdesc::xml_pack_t& targ, const classdesc::string& desc, C& c, T arg) {} 
/* const static members */
template<class T>
void xml_pack(classdesc::xml_pack_t& targ, const classdesc::string& desc, 
              classdesc::is_const_static i, T arg) 
{} 

template<class T, class U>
void xml_pack(classdesc::xml_pack_t& targ, const classdesc::string& desc, 
              classdesc::is_const_static i, const T&, U) {} 

template<class T>
void xml_pack(classdesc::xml_pack_t& targ, const classdesc::string& desc, 
              classdesc::Exclude<T>&) {} 

// special handling of shared pointers to avoid a double wrapping problem
template<class T>
void xml_pack(classdesc::xml_pack_t& x, const classdesc::string& d, 
              classdesc::shared_ptr<T>& a);

namespace classdesc
{
  template<class T>
  void xml_pack(xml_pack_t& targ, const string& desc, is_graphnode, T&)
  {
    throw exception("xml_pack of arbitrary graphs not supported");
  } 

}

#endif
