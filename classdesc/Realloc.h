/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* 
   Reimplement realloc to be be non memory-leaking version of what the
   man page says. By redefining Realloc, the user can replace this by
   a version of their choice.
*/
#include <iostream>
#ifndef REALLOC_H
#define REALLOC_H
#include <cstdlib>
namespace classdesc
{
  inline void *realloc(void *x, std::size_t s)
  {
    if (s && x) 
      return std::realloc(x,s);
    else if (s)
      return std::malloc(s);
    else
      std::free(x);
    return NULL;
  }
}
#endif
