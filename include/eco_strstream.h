/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
  \brief An EcoLab string stream class
*/
/* 

In particular, operator<< adds spaces in between its arguments, and a
new operator| is defined that is similar to strstream operator<<. This
makes it easier to construct TCL commands.

Note that | has higher precedence than <<, so when mixing the two,
ensure that no << operator appears to the right of |, or use brackets
to ensure the correct interpretaion:

eg.
    s << a << b | c;
or
    (s | a) << b | c;

but not
    s | a << b | c;

In any case, you'll most likely get a compiler warning if you do the
wrong thing.

eco_strstream is derived from ostringstream class, so has all the ostringstream
behaviour, aside from the above variant streaming behaviour.

*/

#ifndef ECO_STRSTREAM_H
#define ECO_STRSTREAM_H

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "error.h"
#include "classdesc.h"

namespace ecolab
{
  using namespace classdesc;
  // we do this to define a catch-all operator<< that doesn't leak
  using std::operator<<;
  template<class T, class charT, class Traits>
  std::basic_ostream<charT,Traits>& operator<<
    (std::basic_ostream<charT,Traits>& o, const T& x)
  {throw error("operator<< not defined for %s",typeid(T).name());}

  /// An EcoLab string stream class
  /** 
     This class inserts spaces between successive write, so is more
     useful for TCL use. To concatenate arguments, use operator|
   */
  class eco_strstream: public std::ostringstream
  {
  public:
    eco_strstream() {}
    eco_strstream(const eco_strstream& x)
    {(*this) << x.str();} 
    eco_strstream(const std::ostringstream& x)
    {(*this) << x.str();} 

    /* 
       some implementations of ostringstream do not provide explicit 
       definitions character string constants
    */
    eco_strstream& operator|(const char* const& x) 
    {(*static_cast<std::ostringstream*>(this))<<const_cast<const char*>(x); return *this;}
    eco_strstream& operator|(char* const& x) 
    {(*static_cast<std::ostringstream*>(this))<<const_cast<const char*>(x); return *this;}

    template <class E>
    typename classdesc::enable_if<is_enum<E>, eco_strstream&>::T
    operator|(E x)
    {
      return operator|(enum_keys<E>()(x));
    }

    template <class T>
    typename classdesc::enable_if<Not<is_enum<T> >, eco_strstream&>::T
    operator|(const T& x) 
    {(*static_cast<std::ostringstream*>(this))<<x; return *this;}

    template<class T>
    eco_strstream& operator<<(const T& x) 
    {
      if (this->str()[0]=='\0')
	return (*this)|x;
      else
	return (*this)|' '|x;
    }

    void clear() {std::ostringstream::str(std::string());}

  };

  inline std::ostream& operator<<(std::ostream& x, const eco_strstream& y)
  {return x<<y.str();}
}

#endif
