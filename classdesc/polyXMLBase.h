/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLYXMLBASE_H
#define POLYXMLBASE_H

#include "xml_pack_base.h"
#include "xml_unpack_base.h"

namespace classdesc
{
  struct PolyXMLBase
  {
    virtual void xml_pack(xml_pack_t&, const string&) const=0;
    virtual void xml_unpack(xml_unpack_t&, const string&)=0;
    virtual ~PolyXMLBase() {}
  };

  template <class T>
  struct PolyXML: virtual public PolyXMLBase
  {
    void xml_pack(xml_pack_t& x, const string& d) const
    //{::xml_pack(x,d,static_cast<const T&>(*this));}
    // call access_pack to avoid double wrapping by the XML tag
    {
      classdesc_access::access_xml_pack<T>()
        (x,d,const_cast<T&>(static_cast<const T&>(*this)));
    }
      
    void xml_unpack(xml_unpack_t& x, const string& d)
    {::xml_unpack(x,d,static_cast<T&>(*this));}

  };

  template <> struct tn<PolyXMLBase>
  {
    static std::string name()
    {return "classdesc::PolyXMLBase";}
  };
  template <class T> struct tn<PolyXML<T> >
  {
    static std::string name()
    {return "classdesc::PolyXML<"+typeName<T>()+">";}
  };
}

#endif
      
