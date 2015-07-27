/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef POLYJSONBASE_H
#define POLYJSONBASE_H


namespace classdesc
{
  struct PolyJsonBase
  {
    virtual void json_pack(json_pack_t&, const string&) const=0;
    virtual void json_unpack(json_unpack_t&, const string&)=0;
    virtual ~PolyJsonBase() {}
  };

  template <class T>
  struct PolyJson: virtual public PolyJsonBase
  {
    void json_pack(json_pack_t& x, const string& d) const
    {::json_pack(x,d,dynamic_cast<const T&>(*this));}
      
    void json_unpack(json_unpack_t& x, const string& d)
    {::json_unpack(x,d,static_cast<T&>(*this));}

  };

  template <> struct tn<PolyJsonBase>
  {
    static std::string name()
    {return "classdesc::PolyJsonBase";}
  };
  template <class T> struct tn<PolyJson<T> >
  {
    static std::string name()
    {return "classdesc::PolyJson<"+typeName<T>()+">";}
  };
}

#endif
      
