/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef XSD_GENERATE_BASE
#define XSD_GENERATE_BASE
#include "classdesc.h"
#include <map>
#include <set>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace classdesc
{
  class xsd_generate_t
  {
    std::map<string, string> xsdDefs;
    std::map<string, std::set<string> > dependencies;

    struct TypeBeingAddedTo
    {
      bool complete; //set to true if current type definition is complete
      bool sequenceAdded;
      string name, description, baseClass;
      TypeBeingAddedTo(const string& name="", const string& d="", bool complete=false): 
        complete(complete), sequenceAdded(false), name(name), description(d) {}      
    };
      
    std::vector<TypeBeingAddedTo> typeBeingaddedTo;
    std::set<string> written; // record when a type is written

    void outputType(std::ostream& o, const string& type)
    {
      if (!written.insert(type).second) return; //already done
      // ensure dependencies are written
      const std::set<string>& deps=dependencies[type];
      for (std::set<string>::const_iterator d=deps.begin(); d!=deps.end(); ++d)
        outputType(o, *d);
      o<<xsdDefs[type];
    }

  public:
    /// name of the root element, and its XSD type, in the Schema
    string rootName, rootType;
    /// set to true if next addMember refers to an optional element
    bool optional;

    /// RAII helper class to set optional to \a opt for current scope
    struct Optional
    {
      xsd_generate_t& x;
      bool prev_opt;
      Optional(xsd_generate_t& x, bool o): x(x), prev_opt(x.optional) {x.optional=o;}
      ~Optional() {x.optional=prev_opt;}
    };

    xsd_generate_t(): optional(false) {}

    /// add an attribute \a name with XSD type \a memberType
    void addMember(const string& name, const string& memberType)
    {
      if (!name.empty() && 
          !typeBeingaddedTo.empty() && !typeBeingaddedTo.back().complete)
        {
          if (!typeBeingaddedTo.back().sequenceAdded)
            xsdDefs[typeBeingaddedTo.back().name]+="    <xs:sequence>\n";
          typeBeingaddedTo.back().sequenceAdded=true;
          xsdDefs[typeBeingaddedTo.back().name]+=
            "      <xs:element name=\""+name+"\" type=\""
            +memberType+(optional?"\" minOccurs=\"0":"")+"\"/>\n";
          addDependency(typeBeingaddedTo.back().name, memberType);
        }
    }

    /// add a base class to the current definition
    void addBase(const string& base)
    {
      if (!typeBeingaddedTo.empty() && !typeBeingaddedTo.back().complete)
        {
          if (typeBeingaddedTo.back().baseClass.empty())
            {
              xsdDefs[typeBeingaddedTo.back().name]+=
                "      <xs:complexContent>\n"
                "      <xs:extension base=\""+base+"\">\n";
              typeBeingaddedTo.back().baseClass=base;
            }
          else if (typeBeingaddedTo.back().baseClass!=base)
            throw exception
              ("Multiple inheritance not supported: "+typeBeingaddedTo.back().name);
        }
    }

    /// add a dependency between \a type and \a dependency (XSD qualifed name)
    void addDependency(const string& type, const string& dependency)
    {
      // dependencies only exist in target namespace
      if (dependency.substr(0,4)=="tns:")
        dependencies[type].insert(dependency.substr(4));
    }

    /// start collecting attribute names for \a type.  \a description
    /// is the description string passed through as arg2 of
    /// xsd_generate()
    void openType(const string& type, const string& description)
    {
      typeBeingaddedTo.push_back
        (TypeBeingAddedTo(type, description, xsdDefs.count(type)>0));
      if (!typeBeingaddedTo.back().complete)
        xsdDefs[type]="  <xs:complexType name=\""+type+"\">\n";
    }
    /// complete type definition - matching last nested openType
    void closeType()
    {
      if (!typeBeingaddedTo.empty() && !typeBeingaddedTo.back().complete)
        {
          // allow schema to be extensible - either for polymorphic
          // reasons, or for future extensibility
          xsdDefs[typeBeingaddedTo.back().name]+=
            "      <xs:any minOccurs=\"0\" "
            "maxOccurs=\"unbounded\" processContents=\"lax\"/>\n";
          if (typeBeingaddedTo.back().sequenceAdded)
            xsdDefs[typeBeingaddedTo.back().name]+="    </xs:sequence>\n";
          if (!typeBeingaddedTo.back().baseClass.empty())
            xsdDefs[typeBeingaddedTo.back().name]+="      </xs:extension>\n"
              "      </xs:complexContent>\n";
          xsdDefs[typeBeingaddedTo.back().name]+="  </xs:complexType>\n";
        }
      typeBeingaddedTo.pop_back();
    }

    string currentDescription() const 
    {
      if (!typeBeingaddedTo.empty())
        return typeBeingaddedTo.back().description;
      else
        return "";
    }

    /// add a complete XSD definition for \a type
    void defineType(const string& type, const string& def)
    {
      if (xsdDefs.count(type)==0)
        xsdDefs[type]=def;
    }

    /**
       output XSD to the stream \o, targetting XML namespace \a targetNS
    */
    void output(std::ostream& o, const string& targetNS)
    {
      written.clear(); 
      o<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      o<<"<xs:schema  targetNamespace=\"";
      o<<targetNS;
      o<<"\"\n xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"\n";
      o<<" xmlns:tns=\""<<targetNS<<"\"\n"; 
      o<<" elementFormDefault=\"qualified\" attributeFormDefault=\"unqualified\">\n";
      o<<"\n";
      for (std::map<string, string>::const_iterator i=xsdDefs.begin();
           i!=xsdDefs.end(); ++i)
        outputType(o, i->first);
      o<<"\n";
      o<<"  <xs:element name=\""<<rootName<<"\" type=\""<<rootType<<"\"/>\n";
      o<<"</xs:schema>\n";
    }
  };

  template <class T> struct UnknownSchema: public exception
  {
    string msg;
    UnknownSchema(): msg("unknown schema for "+typeName<T>()) {}
    ~UnknownSchema() throw() {}
    const char* what() const throw() 
    {return msg.c_str();}
  };

}

namespace classdesc_access
{
  template <class T> struct access_xsd_generate
  {
    // by default, throw an exception
    void operator()(classdesc::xsd_generate_t&,const classdesc::string&,T&)
    {throw classdesc::UnknownSchema<T>();}
  };
}


namespace classdesc
{
  // replace all occurances of non-acceptable characters with _
  inline string transformTypeName(string x)
  {
    for (string::size_type i=0; i<x.length(); ++i)
      if (!isalnum(x[i]))
        x[i]='_';
    return x;
  }

  template <class T> string xsd_typeName() 
  {return "tns:"+transformTypeName(typeName<T>());}

  template <class T>
  struct EverythingElse: public
  Not<
    Or<
      Or<is_fundamental<T>,is_container<T> >,
      is_enum<T>
      >
    >{};

  template <class T>
  typename enable_if<EverythingElse<T>, void>::T
  xsd_generate(xsd_generate_t& g, const string& d, const T& a)
  {
    // if this is a toplevel name, record it as the root element
    if (!d.empty() && d.find('.')==string::npos)
      {
        g.rootName=d;
        g.rootType=xsd_typeName<T>();
      }

    if (g.currentDescription()==d) // this is a base class, not a member
      g.addBase(xsd_typeName<T>());
    else
      g.addMember(tail(d),xsd_typeName<T>());
    g.openType(transformTypeName(typeName<T>()), d);
    classdesc_access::access_xsd_generate<T>()(g,d,const_cast<T&>(a));
    g.closeType();
  }

  

  template <> inline string xsd_typeName<bool>() {return "xs:boolean";}
  template <> inline string xsd_typeName<char>() {return "xs:string";}
  template <> inline string xsd_typeName<signed char>() {return "xs:string";}
  template <> inline string xsd_typeName<short>() {return "xs:short";}
  template <> inline string xsd_typeName<int>() {return "xs:int";}
  template <> inline string xsd_typeName<long>() {return "xs:long";}
  template <> inline string xsd_typeName<unsigned char>() {return "xs:string";}
  template <> inline string xsd_typeName<unsigned short>() {return "xs:unsignedShort";}
  template <> inline string xsd_typeName<unsigned int>() {return "xs:unsignedInt";}
  template <> inline string xsd_typeName<unsigned long>() {return "xs:unsignedLong";}

#ifdef HAVE_LONGLONG
  template <> inline string xsd_typeName<long long>() {return "xs:long";}
  template <> inline string xsd_typeName<unsigned long long>() {return "xs:unsignedLong";}
#endif

  template <> inline string xsd_typeName<float>() {return "xs:float";}
  template <> inline string xsd_typeName<double>() {return "xs:double";}
  // long double is not explicitly supported in XSD, so overflows may result
  template <> inline string xsd_typeName<long double>() {return "xs:double";}
  template <> inline string xsd_typeName<string>() {return "xs:string";}
  template <> inline string xsd_typeName<std::wstring>() {return "xs:string";}

  template <class T> 
  typename enable_if<is_fundamental<T>, void>::T
  xsd_generate(xsd_generate_t& g, const string& d, const T& a)
  {g.addMember(tail(d),xsd_typeName<T>());}

  template <class T> 
  void xsd_generate(xsd_generate_t& g, const string& d, const std::basic_string<T>& a)
  {g.addMember(tail(d),"xs:string");}

  /// array handling
  template <class T> 
  void xsd_generate
  (xsd_generate_t& g, const string& d, is_array ia, T& a, 
   int dims, size_t ncopies, ...) 
  {
    std::ostringstream type;
    type << "__builtin_array_"+transformTypeName(typeName<T>())<<"_"<<ncopies;
    va_list ap;
    va_start(ap,ncopies);
    for (int i=1; i<dims; i++) 
      {
        //assume that 2 and higher D arrays dimensions are int
        int dim=va_arg(ap,int);
        ncopies*=dim;
        type<<"_"<<dim;
      }
    va_end(ap);
    
    std::ostringstream os;
    os<<"  <xs:complexType name=\""+type.str()+"\">\n";
    os<<"    <xs:sequence minOccurs=\""<<ncopies<<
      "\" maxOccurs=\""<<ncopies<<"\">\n";
    os<<"      <xs:element name=\""<<typeName<T>()<<"\" type=\""<<
      xsd_typeName<T>()<<"\"/>\n";
    os<<"    </xs:sequence>\n";
    os<<"  </xs:complexType>\n";

    g.defineType(type.str(), os.str());
    g.addMember(tail(d), "tns:"+type.str());
    g.addDependency(type.str(), xsd_typeName<T>());
  }

  /// enum handling
  template <class E> 
  typename enable_if<is_enum<E>, void>:: T
  xsd_generate(xsd_generate_t& g, const string& d, const E& arg)
  {
    // const_cast OK here, xsd_generate is a read-only operation
    xsd_generate(g, d, Enum_handle<E>(const_cast<E&>(arg)));
  }

  
  template <class E> 
  void xsd_generate(xsd_generate_t& g, const string& d, const Enum_handle<E>& e)
  {
    string type=transformTypeName(typeName<E>());
    std::ostringstream os;
    os << "  <xs:simpleType name=\""<<type<<"\">\n";
    os << "    <xs:restriction base=\"xs:string\">\n";
    for (typename EnumKeys<E>::iterator i=enum_keysData<E>::keys.begin();
         i!=enum_keysData<E>::keys.end(); ++i)
      os << "      <xs:enumeration value=\""<<i->second<<"\"/>\n";
    os << "    </xs:restriction>\n";
    os << "  </xs:simpleType>\n";
    g.defineType(type, os.str());
    g.addMember(tail(d), xsd_typeName<E>());
  }

  // container handling
  template <class T> 
  typename enable_if<is_container<T>, void>::T
  xsd_generate(xsd_generate_t& g, const string& d, const T& e)
  {    
    std::ostringstream os;
    // element name is given by the type name
    string eName=typeName<typename T::value_type>().c_str();
    eName=eName.substr(0,eName.find('<')); //trim off any template args
    // strip leading namespace and qualifiers
    const char *el=eName.c_str()+eName.length();
    while (el!=eName.c_str() && *(el-1)!=' ' && *(el-1)!=':') el--;

    string type=transformTypeName(typeName<T>());
    os << "  <xs:complexType name=\"" << type << "\">\n";
    os << "    <xs:sequence minOccurs=\"0\" maxOccurs=\"unbounded\">\n";
    os << "      <xs:element name=\""<<el<<
      "\" type=\""<<xsd_typeName<typename T::value_type>()<<"\"/>\n";
    os << "    </xs:sequence>\n";
    os << "  </xs:complexType>\n";
    g.addMember(tail(d), xsd_typeName<T>());
    g.defineType(type, os.str());
    g.addDependency(type, xsd_typeName<typename T::value_type>());
    // ensure that the value type as a definition also
    xsd_generate(g,"",typename T::value_type());
  }

  // support for maps
  template <class T, class U>
  void xsd_generate(xsd_generate_t& g, const string& d, const std::pair<T,U>& a)
  {
    g.openType(transformTypeName(typeName<std::pair<T,U> >()), d);
    xsd_generate(g,d+".first",a.first);
    xsd_generate(g,d+".second",a.second);
    g.closeType();
  }

  template <class T>
  void xsd_generate(xsd_generate_t& g, const string& d, const Exclude<T>& a) {}

  //  member functions
  template <class C, class T>
  void xsd_generate(xsd_generate_t& g, const string& d, C& c, const T& a) {}

  template <class T>
  void xsd_generate(xsd_generate_t& g, const string& d, is_const_static, T a) {}

  template <class T, class U>
  void xsd_generate(xsd_generate_t& g, const string& d, is_const_static, const T&, U) {}
   
  // with shared_ptrs, just write out the schema for the base
  // class. Additional data may included in the XML file, but in
  // general, XML does not support polymorphism, so this won't really
  // work anyway.
  template <class T>
  void xsd_generate(xsd_generate_t& g, const string& d, const shared_ptr<T>& a) 
  {
    xsd_generate_t::Optional o(g,true); 
    xsd_generate(g,d,*a);
  }

  template <class T>
  void xsd_generate_onbase(xsd_generate_t& g, const string& d, T a) 
  {xsd_generate(g,d+basename<T>(),a);}

}
    
using classdesc::xsd_generate;
using classdesc::xsd_generate_onbase;

#endif
