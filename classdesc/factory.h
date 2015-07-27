/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef FACTORY_H
#define FACTORY_H
#include "classdesc.h"
#include "error.h"
#include <sstream>

namespace classdesc
{
  /// factory template for a class heirarchy. \a B is the base class,
  /// and \a Type is the type class
  template <class B, class Type>
  class Factory
  {
    struct CreatorBase
    {
      virtual B* create() const=0;
    };
    typedef std::map<Type, shared_ptr<CreatorBase> > Fmap;
    Fmap fmap;

    template <class T>
    struct Creator: public CreatorBase
    {
      B* create() const {return new T();}
    };
  public:

    /// register type T for use with this factory
    template <class T>
    void registerType(const Type& t)
    {
      fmap.insert
        (make_pair(t, shared_ptr<CreatorBase>(new Creator<T>)));
    }
    /// convenience method for registering a type. 
    template <class T> void registerType()
    {registerType<T>(T().type());}


    /**
       Users of this template must define a constructor that registers
       all the types in the heirarchy.

       For example, assuming Foo and Bar are subtypes of FooBase:
       template <> Factory<Foobase>::Factory()
       {
         registerType<Foo>();
         registerType<Bar>();
       };
    */

    Factory();

    struct InvalidType: public exception
    {
      string s;
      InvalidType(const Type& t) {
        std::ostringstream os;
        os<<"invalid type "<<t;
        s=os.str();
      }
      const char* what() const throw() {return s.c_str();}
      ~InvalidType() throw() {}
    };

    /// Create a default constructed object given by the type string 
    B* create(const Type& t) const
    {
      typename Fmap::const_iterator i=fmap.find(t);
      if (i==fmap.end())
        throw InvalidType(t);
      else
        return i->second->create();
    }

    /// return list of registered types
    std::vector<string> types() const
    {
      std::vector<string> r;
      for (typename Fmap::const_iterator i=fmap.begin(); i!=fmap.end(); ++i)
        r.push_back(i->first);
      return r;
    }

  };
}

#endif
