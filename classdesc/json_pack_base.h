/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef JSON_PACK_BASE_H
#define JSON_PACK_BASE_H
#include "classdesc.h"
#include <json_spirit.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <map>

namespace classdesc
{
  class json_pack_error : public exception 
  {
    static const int maxchars=200; /* I hope this will always be large enough */
    char errstring[maxchars];
  public:
    json_pack_error(const char *fmt,...)
     {
       va_list args;
       va_start(args, fmt);
       vsnprintf(errstring,maxchars,fmt,args);
       va_end(args);
     }
    virtual ~json_pack_error() throw() {}
    virtual const char* what() const throw() {return errstring;}
  };

  // these are classes, not typedefs to avoid adding properties to mValue
  class json_pack_t: public json_spirit::mValue
  {
  public:
    bool throw_on_error; ///< enable exceptions on error conditions
    json_pack_t(): json_spirit::mValue(json_spirit::mObject()), 
                   throw_on_error(false)  {}
    json_pack_t(const json_spirit::mValue& x): json_spirit::mValue(x), 
                                               throw_on_error(false) {}
  };


  typedef json_pack_t json_unpack_t;

  /// forward declare generic json operations
  template <class T> void json_pack(json_pack_t& o, const string& d, T& a);

  template <class T> void json_pack(json_pack_t& o, const string& d, const T& a)
  {json_pack(o,d,const_cast<T&>(a));}
 
  template <class T> void json_unpack(json_unpack_t& o, const string& d, T& a);

  template <class T> json_pack_t& operator<<(json_pack_t& j, const T& a) 
  {json_pack(j,"",a); return j;}

  template <class T> const json_unpack_t& operator>>(const json_unpack_t& j, T& a) 
  {json_unpack(const_cast<json_unpack_t&>(j),"",a); return j;}

  /// find an object named by \a name within the json object x
  inline json_spirit::mValue& 
  json_find(json_spirit::mValue& x, std::string name)
  {
    if (name.size()==0) return x;
    if (name[0]=='.') name.erase(0,1); //remove leading '.'
    std::string::size_type p=name.find('.');
    if (x.type()==json_spirit::obj_type)
      {
        json_spirit::mObject& xo=x.get_obj();
        json_spirit::mObject::iterator i=xo.find(name.substr(0,p));
        if (i==xo.end())
          throw json_pack_error("json object %s not found", name.substr(0,p).c_str());
        else if (p==std::string::npos)
          return i->second;
        else
          return json_find(i->second,name.substr(p,std::string::npos));
      }
    else
      throw json_pack_error("%s is not a json object",name.c_str());
  }

  //json_spirit::mValue does not provide constructors for everything. Oh well..
  template <class T> json_spirit::mValue valueof(T a) 
  {return json_spirit::mValue(a);}
  template <class T> T getValue(const json_spirit::mValue& x) 
  {return x.get_value<T>();} 

  inline json_spirit::mValue valueof(unsigned char a)  
  {return json_spirit::mValue(int(a));}
  template <> inline unsigned char getValue(const json_spirit::mValue& x) 
  {return x.get_value<int>();}

  inline json_spirit::mValue valueof(signed char a)  
  {return json_spirit::mValue(int(a));}
  template <> inline signed char getValue(const json_spirit::mValue& x) 
  {return x.get_value<int>();}

  inline json_spirit::mValue valueof(char a)  
  {return json_spirit::mValue(int(a));}
  template <> inline char getValue(const json_spirit::mValue& x) 
  {return x.get_value<int>();}

  inline json_spirit::mValue valueof(unsigned short a)  
  {return json_spirit::mValue(int(a));}
  template <> inline unsigned short getValue(const json_spirit::mValue& x) 
  {return x.get_value<int>();}

  inline json_spirit::mValue valueof(signed short a)  
  {return json_spirit::mValue(int(a));}
  template <> inline signed short getValue(const json_spirit::mValue& x) 
  {return x.get_value<int>();}

  inline json_spirit::mValue valueof(unsigned int a)  
  {return json_spirit::mValue(boost::uint64_t(a));}
  template <> inline unsigned getValue(const json_spirit::mValue& x) 
  {return x.get_value<boost::uint64_t>();}

  inline json_spirit::mValue valueof(unsigned long a)  
  {return json_spirit::mValue(boost::uint64_t(a));}
  template <> inline unsigned long getValue(const json_spirit::mValue& x) 
  {return x.get_value<boost::uint64_t>();}

  inline json_spirit::mValue valueof(long a)  
  {return json_spirit::mValue(boost::int64_t(a));}
  template <> inline long getValue(const json_spirit::mValue& x) 
  {return x.get_value<boost::int64_t>();}

#ifdef HAVE_LONGLONG
  inline json_spirit::mValue valueof(unsigned long long a)  
  {return json_spirit::mValue(boost::uint64_t(a));}
  template <> inline unsigned long long getValue(const json_spirit::mValue& x) 
  {return x.get_value<boost::uint64_t>();}

  inline json_spirit::mValue valueof(long long a)  
  {return json_spirit::mValue(boost::int64_t(a));}
  template <> inline long long getValue(const json_spirit::mValue& x) 
  {return x.get_value<boost::int64_t>();}
#endif

  inline json_spirit::mValue valueof(float a)  
  {return json_spirit::mValue(double(a));}
  template <> inline float getValue(const json_spirit::mValue& x) 
  {return x.get_value<double>();}

 // basic types
  template <class T> typename
  enable_if<Or<is_fundamental<T>,is_string<T> >, void>::T
  json_packp(json_unpack_t& o, const string& d, const T& a, dummy<0> dum=0)
  {
    using namespace json_spirit;
    if (d=="")
      o=valueof(a);
    else
      {
        try
          {
            json_spirit::mValue& parent=json_find(o,head(d));
            if (parent.type()==obj_type)
              parent.get_obj()[tail(d)]=valueof(a);
            else
              throw json_pack_error("cannot add to a basic type");
          }
        catch (json_pack_error&)
          {
            // only throw if this flag is set
            if (o.throw_on_error) throw; 
          }
      }
   } 

  // basic types
  template <class T> typename
  enable_if<Or<is_fundamental<T>,is_string<T> >, void>::T
  json_unpackp(json_unpack_t& o, string d, T& a, dummy<0> dum=0)
  {
    try
      {
        a=getValue<T>(json_find(o,d));
      }
    catch (const json_pack_error&)
      {
        // only throw if this flag is set
        if (o.throw_on_error) throw; 
      }
  }  

  template <class T> void json_pack_isarray
  (json_spirit::mValue& jval, const T& val, std::vector<size_t> dims) 
  {
    if (dims.empty())
      {
        json_pack_t j;
        json_pack(j,"",val);
        jval=j;
      }
    else
      {
        size_t s=dims.back();
        jval=json_spirit::mArray(s);
        dims.pop_back();
        size_t stride=1;
        for (size_t i=0; i<dims.size(); ++i) stride*=dims[i];
        for (size_t i=0; i<s; ++i)
          json_pack_isarray(jval.get_array()[i],(&val)[i*stride], dims);
      }
  }
                                            
  // array handling
  template <class T>
  void json_pack(json_pack_t& o, const string& d, is_array ia, const T& a, 
            int ndims,size_t ncopies,...)
  {
  va_list ap;
  va_start(ap,ncopies);
  std::vector<size_t> dims(ndims);
  dims[ndims-1]=ncopies;
  for (int i=ndims-2; i>=0; --i) dims[i]=va_arg(ap,size_t);
  va_end(ap);
  try
    {
      json_spirit::mValue& parent=json_find(o,head(d));
      if (parent.type()!=json_spirit::obj_type)
        throw json_pack_error("attempt to pack an array member into a non-object");
      else
        json_pack_isarray(parent.get_obj()[tail(d)],a,dims);
    }
    catch (json_pack_error&)
      {
        // only throw if this flag is set
        if (o.throw_on_error) throw; 
      }
  }
 
  template <class T> void json_unpack_isarray
  (const json_spirit::mValue& jval, T& val, std::vector<size_t> dims) 
  {
    if (dims.empty())
      {
        json_unpack_t j(jval);
        json_unpack(j,"",val);
      }
    else
      {
        size_t s=dims.back();
        dims.pop_back();
        size_t stride=1;
        for (size_t i=0; i<dims.size(); ++i) stride*=dims[i];
        for (size_t i=0; i<s; ++i)
          json_unpack_isarray(jval.get_array()[i],(&val)[i*stride], dims);
      }
  }
                                            
  template <class T>
  void json_unpack(json_unpack_t& o, const string& d, is_array ia, T& a, 
            int ndims,size_t ncopies,...)
  {
    va_list ap;
    va_start(ap,ncopies);
    std::vector<size_t> dims(ndims);
    dims[ndims-1]=ncopies;
    for (int i=ndims-2; i>=0; --i) dims[i]=va_arg(ap,size_t);
    va_end(ap);
    try
      {
        const json_spirit::mValue& v=json_find(o,d);
        if (v.type()!=json_spirit::array_type)
          throw json_pack_error
            ("attempt to unpack an array member from a non-object");
        else 
          json_unpack_isarray(v,a,dims);
      }
    catch (json_pack_error&)
      {
        // only throw if this flag is set
        if (o.throw_on_error) throw; 
      }
    
  }

    
  /**
     handle enums
  */

  template <class T> void json_pack(json_pack_t& x, const string& d,
                                    Enum_handle<T> arg)
  {
    string tmp(static_cast<string>(arg));
    json_pack(x,d,tmp);
  }

  //Enum_handles have reference semantics
  template <class T> void json_unpack(json_unpack_t& x, const string& d,
                                    Enum_handle<T> arg)
  {
    std::string tmp;
    json_unpack(x,d,tmp);
    // remove extraneous white space
    int (*isspace)(int)=std::isspace;
    std::string::iterator end=std::remove_if(tmp.begin(),tmp.end(),isspace);
    arg=tmp.substr(0,end-tmp.begin());
  }

  /** standard container handling */
  template <class T> typename
  enable_if<is_sequence<T>, void>::T
  json_unpackp(json_unpack_t& o, const string& d, T& a, dummy<1> dum=0)
  {
    try
      {
        const json_spirit::mValue& val=json_find(o,d);
        if (val.type()!=json_spirit::array_type)
          throw json_pack_error("%s is not an array",d.c_str());
        else
          {
            const json_spirit::mArray& arr=val.get_array();
            for (size_t i=0; i<arr.size(); ++i)
              {
                typename T::value_type v;
                json_unpack_t j(arr[i]);
                json_unpack(j,"",v);
                a.push_back(v);
              }
          }
      }
    catch (json_pack_error&)
      {
        if (o.throw_on_error) throw;
      }
  }

  template <class T1, class T2> 
  void json_pack(json_pack_t& o, const string& d, std::pair<T1,T2>& a)
  {
    json_pack(o,d+".first",a.first);
    json_pack(o,d+".second",a.second);
  }

  template <class T1, class T2>
  void json_unpackp(json_unpack_t& o, const string& d, std::pair<T1,T2>& a)
  {
    json_unpack(o,d+".first",a.first);
    json_unpack(o,d+".second",a.second);
  }

  template <class T> typename
  enable_if<Or<is_sequence<T>,is_associative_container<T> >, void>::T
  json_packp(json_pack_t& o, const string& d, const T& a, dummy<1> dum=0)
  {
  try
    {
      json_spirit::mValue& parent=json_find(o,head(d));
      if (parent.type()!=json_spirit::obj_type)
        throw json_pack_error("attempt to pack an array member into a non-object");
      else
        {
          json_spirit::mValue* v;
          if (d.empty())
            v=&parent;
          else
            v=&parent.get_obj()[tail(d)];

          json_spirit::mArray& arr=
            (*v=json_spirit::mArray(a.size())).get_array();
          typename T::const_iterator i=a.begin();
          for (size_t k=0; i!=a.end(); ++i, ++k)
            {
              json_pack_t j;
              json_pack(j,"",*i);
              arr[k]=j;
            }
        }
    }
  catch (json_pack_error&)
    {
      if (o.throw_on_error) throw;
    }
  }

  template <class T> typename
  enable_if<is_associative_container<T>, void>::T
  json_unpackp(json_unpack_t& o, const string& d, T& a, dummy<2> dum=0)
  {
    try
      {
        const json_spirit::mValue& val=json_find(o,d);
        if (val.type()!=json_spirit::array_type)
          throw json_pack_error("%s is not an array",d.c_str());
        else
          {
            const json_spirit::mArray& arr=val.get_array();
            for (size_t i=0; i<arr.size(); ++i)
              {
                typename NonConstKeyValueType<typename T::value_type>::T v;
                json_unpack_t j(arr[i]);
                json_unpack(j,"",v);
                a.insert(v);
              }
          }
      }
    catch (json_pack_error&)
      {
        if (o.throw_on_error) throw;
      }
  }

  /*
    Method pointer serialisation (do nothing)
  */
  template <class C, class T>
  void json_pack(json_pack_t& targ, const string& desc, C& c, T arg) {}

  template <class C, class T>
  void json_unpack(json_unpack_t& targ, const string& desc, C& c, T arg) {}

  /*
    const static support
  */
  template <class T>
  void json_pack(json_pack_t& targ, const string& desc, is_const_static i, T arg) 
  {}

  template <class T>
  void json_unpack(json_unpack_t& targ, const string& desc, is_const_static i, T arg) 
  {}

  // static methods
  template <class T, class U>
  void json_pack(json_pack_t&, const string&, is_const_static, const T&, U) {}

  // static methods
  template <class T, class U>
  void json_unpack(json_unpack_t&, const string&, is_const_static, const T&, U) {} 

  template <class T>
  void json_unpack(json_unpack_t& targ, const string& desc, const T& arg) {}

  template <class T>
  void json_pack(json_pack_t& targ, const string& desc, Exclude<T>& arg) {}

  template <class T>
  void json_unpack(json_unpack_t& targ, const string& desc, Exclude<T>& arg) {} 

  /// produce json string equivalent of object \a x
  template <class T> string json(const T& x) 
  {
    json_pack_t j;
    json_pack(j,"",x);
    return write(j);
  }
  template <class T> void json(const T& x, const string& s) 
  {
    json_pack_t j;
    read(s, j);
    json_unpack(j,"",x);
  }

  template <class T>
  void json_pack_onbase(json_pack_t& x,const string& d,T& a)
  {json_pack(x,d+basename<T>(),a);}

  template <class T>
  void json_unpack_onbase(json_unpack_t& x,const string& d,T& a)
  {json_unpack(x,d+basename<T>(),a);}


}

namespace classdesc_access
{
  template <class T> struct access_json_pack;
  template <class T> struct access_json_unpack;
}

using classdesc::json_pack_onbase;
using classdesc::json_unpack_onbase;

using classdesc::json_pack;
using classdesc::json_unpack;
#endif
