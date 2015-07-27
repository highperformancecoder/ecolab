/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef PACK_STREAM_H
#define PACK_STREAM_H
#include "pack_base.h"

namespace classdesc
{
  template <class T>  
  pack_t& operator<<(pack_t& y,const T&x)
  {::pack(y,string(),x); return y;}

  template <class T>
  pack_t& operator>>(pack_t& y,T&x) 
  {::unpack(y,string(),x); return y;}

}

#ifdef CLASSDESCMP_H
namespace classdesc {
  template <class T> inline MPIbuf& MPIbuf::operator<<(const T&x) 
  {::pack((MPIbuf_base&)*this,string(),const_cast<T&>(x)); return *this;}
}

#endif
#endif
