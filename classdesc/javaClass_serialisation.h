/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief serialisation for javaClass
*/
#ifndef JAVACLASS_SERIALISATION_H
#define JAVACLASS_SERIALISATION_H

#include <vector>
#include <string>
#include <pack_base.h>
#include <dump_base.h>
#include "javaClass.h"


namespace classdesc
{
  template <> void dump(dump_t& buf, const string& d, const u8& a)
  {dump(buf,d,a.v);}
  template <> void dump(dump_t& buf, const string& d, const u4& a)
  {dump(buf,d,a.v);}
  template <> void dump(dump_t& buf, const string& d, const u2& a)
  {dump(buf,d,a.v);}
  template <> void dump(dump_t& buf, const string& d, const u1& a)
  {buf << std::hex << int(a);}
}
 

namespace classdesc_access
{
/*
  Strings and vectors are have specialised serialisation for javaClass 
*/

  template <>
  struct access_pack<std::string>
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, const std::string& a)
    {
      classdesc::u2 length=a.length(); 
      pack(buf,d,length);
      buf.packraw(a.c_str(),length);
    }
  };

  template <>
  struct access_unpack<std::string>
  {
    template <class U>
    void operator()(classdesc::unpack_t& buf, const classdesc::string& d, U& a)
    {
      classdesc::u2 length; 
      unpack(buf,d,length);
      char *s=new char[length];
      buf.unpackraw(s,length);
      a=std::string(s,length);
      delete [] s;
    }
  };

  template <class T>
  struct access_pack<std::vector<T> >
  {
    template <class U>
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, U& a)
    {
      classdesc::u2 length=a.size(); 
      pack(buf,d,length);
      for (int i=0; i<length; i++)
        pack(buf,d,a[i]);
    }
  };

  template <class T>
  struct access_unpack<std::vector<T> >
  {
    template <class U>
    void operator()(classdesc::unpack_t& buf, const classdesc::string& d, U& a)
    {
      classdesc::u2 length; 
      unpack(buf,d,length);
      a.resize(length);
      for (int i=0; i<length; i++)
        unpack(buf,d,a[i]);
    }
  };

  template <>
  struct access_unpack<classdesc::u8>
  {
    template <class U>
    void operator()(classdesc::unpack_t& buf, const classdesc::string& d, U& a)
    {
      a.v=0;
      for (int i=0; i<8; i++)
        {
          classdesc::u1 b;
          unpack(buf,d,b);
          a.v=(a.v<<8) | (0xFF&b);
        }
    }
  };

  template <>
  struct access_pack<classdesc::u8>
  {
    template <class U>
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, U& a)
    {
      for (int i=7; i>=0; i--)
        {
          classdesc::u1 b = a.v >> 8*i;
          pack(buf,d,b);
        }
    }
  };

  template <>
  struct access_unpack<classdesc::u4>
  {
    template <class U>
    void operator()(classdesc::unpack_t& buf, const classdesc::string& d, U& a)
    {
      a.v=0;
      for (int i=0; i<4; i++)
        {
          classdesc::u1 b;
          unpack(buf,d,b);
          a.v=(a.v<<8) | (0xFF&b);
        }
    }
  };

  template <>
  struct access_pack<classdesc::u4>
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, const classdesc::u4& a)
    {
      for (int i=3; i>=0; i--)
        {
          classdesc::u1 b = a.v >> 8*i;
          pack(buf,d,b);
        }
    }
  };

  template <>
  struct access_unpack<classdesc::u2>
  {
    void operator()(classdesc::unpack_t& buf, const classdesc::string& d, classdesc::u2& a)
    {
      classdesc::u1 b1, b2;
      unpack(buf,d,b1);
      unpack(buf,d,b2);
      a=(b1<<8)| (0xFF & b2);
    }
  };

  template <>
  struct access_pack<classdesc::u2>
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, const classdesc::u2& a)
    {
      classdesc::u1 b1=a.v>>8, b2=a.v;
      pack(buf,d,b1);
      pack(buf,d,b2);
    }
  };

  template <>
  struct access_pack<classdesc::cp_info>
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, const classdesc::cp_info& a) 
    {
      using namespace classdesc;
      pack(buf,d,a.tag());
      switch (a.tag())
        {
        case JVM_CONSTANT_Class:
        case JVM_CONSTANT_String: 
          pack(buf,d,a.get<u2>()); break;
        case JVM_CONSTANT_Fieldref: 
        case JVM_CONSTANT_Methodref: 
        case JVM_CONSTANT_InterfaceMethodref: 
          pack(buf,d,a.get<Ref>()); break;
        case JVM_CONSTANT_Integer:
        case JVM_CONSTANT_Float: 
          pack(buf,d,a.get<u4>()); break;
        case JVM_CONSTANT_Long: 
        case JVM_CONSTANT_Double: 
          pack(buf,d,a.get<u8>()); break;
        case JVM_CONSTANT_NameAndType: 
          pack(buf,d,a.get<NameAndTypeInfo>()); 
          break;
        case JVM_CONSTANT_Utf8: 
          pack(buf,d,a.get<std::string>()); break;
        }       
    }
  };

  template <>
  struct access_unpack<classdesc::cp_info>
  {
    void operator()(classdesc::unpack_t& buf, const classdesc::string& d, classdesc::cp_info& a) 
    {
      using namespace classdesc;
      u1 tag;
      unpack(buf,d,tag);
      switch (tag)
        {
        case JVM_CONSTANT_Class:
        case JVM_CONSTANT_String: 
          a.unpack<u2>(buf,tag); break;
        case JVM_CONSTANT_Fieldref: 
        case JVM_CONSTANT_Methodref: 
        case JVM_CONSTANT_InterfaceMethodref: 
          a.unpack<Ref>(buf,tag); break;
        case JVM_CONSTANT_Integer:
        case JVM_CONSTANT_Float: 
          a.unpack<u4>(buf,tag); break;
        case JVM_CONSTANT_Long: 
        case JVM_CONSTANT_Double: 
          a.unpack<u8>(buf,tag); break;
        case JVM_CONSTANT_NameAndType: 
          a.unpack<NameAndTypeInfo>(buf,tag); 
          break;
        case JVM_CONSTANT_Utf8: 
          a.unpack<std::string>(buf,tag); break;
        }       
    }
  };

  // The zeroth element is not serialised in the constant_pool
  template <>
  struct access_pack<std::vector<classdesc::cp_info> >
  {
    template <class U>
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, U& a)
    {
      classdesc::u2 size=a.size(); 
      pack(buf,d,size);
      for (int i=1; i<size; i++)
        pack(buf,d,a[i]);
    }
  };

  // The zeroth element is not serialised in the constant_pool
  template <>
  struct access_unpack<std::vector<classdesc::cp_info> >
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, std::vector<classdesc::cp_info>& a)
    {
      classdesc::u2 size; unpack(buf,d,size);
      a.resize(size);
      for (int i=1; i<size; i++)
        unpack(buf,d,a[i]);
    }
  };

  template <>
  struct access_pack<classdesc::attribute_info>
  {
    template<class U>
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, U& a)
    {
      pack(buf,d,a.attribute_name_index);
      classdesc::u4 length=a.info.size();
      pack(buf,d,length);
      // need to code this explicitly as attribute length is u4 not u2
      for (size_t i=0; i<a.info.size(); i++)
        pack(buf,d,a.info[i]);
    }
  };

  template <>
  struct access_unpack<classdesc::attribute_info>
  {
    void operator()(classdesc::pack_t& buf, const classdesc::string& d, classdesc::attribute_info& a)
    {
      unpack(buf,d,a.attribute_name_index);
      classdesc::u4 length;
      unpack(buf,d,length);
      // need to code this explicitly as attribute length is u4 not u2
      a.info.resize(length);
      for (size_t i=0; i<a.info.size(); i++)
        unpack(buf,d,a.info[i]);
    }
  };

}

/* define this here to take advantage of cp_info's serialisation operators */
inline bool classdesc::cp_info::operator==(const classdesc::cp_info& x) const
{
  pack_t b1, b2;
  pack(b1,string(),*this);
  pack(b2,string(),x);
  return b1.size()==b2.size() && memcmp(b1.data(),b2.data(),b1.size())==0;
}


namespace classdesc
{
  
  void dumpp(dump_t& buf, const string& d, cp_info& a) 
  {
    switch (a.tag())
      {
      case JVM_CONSTANT_Class:
      case JVM_CONSTANT_String: 
        dump(buf,d,a.get<u2>()); break;
      case JVM_CONSTANT_Fieldref: 
      case JVM_CONSTANT_Methodref: 
      case JVM_CONSTANT_InterfaceMethodref: 
        dump(buf,d,a.get<Ref>()); break;
      case JVM_CONSTANT_Integer:
        dump(buf,d,a.get<int>()); break;
      case JVM_CONSTANT_Float: 
        dump(buf,d,a.get<float>()); break;
      case JVM_CONSTANT_Long: 
        dump(buf,d,a.get<long long>()); break;
      case JVM_CONSTANT_Double: 
        dump(buf,d,a.get<double>()); break;
      case JVM_CONSTANT_NameAndType: 
        dump(buf,d,a.get<NameAndTypeInfo>()); 
        break;
      case JVM_CONSTANT_Utf8: 
        dump(buf,d,a.get<std::string>()); break;
      }       
  }
}


namespace classdesc
{
  inline void dumpp(dump_t& targ, const string& desc,struct attribute_info& arg)
  {
    dump(targ,desc+".attribute_name_index",arg.attribute_name_index);
    int tab=format(targ, desc+".info");
    targ << std::setw(tab) << "";
    for (u1 *c=&arg.info[0]; c!=&arg.info[0]+arg.info.size(); c++)
      targ << " "<<std::setw(2)<<std::setfill('0')<<std::hex << int(*c);
    targ<<std::setfill(' ')<<std::endl;
    
  }
}

template <class T> void classdesc::cp_info::unpack(pack_t& t, u1 tag) {
  T tmp;
  ::unpack(t,"",tmp);
  set(tag,tmp);
}


#endif
