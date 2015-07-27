/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief EcoLab exception class
*/
#ifndef ERROR_H
#define ERROR_H

/* error handling */
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

namespace ecolab
{
  /// EcoLab exception class
  class error: public std::exception
  {
    char errstring[200];  /* I hope this will always be large enough */
    void abort_if_debug() 
    {
      /* A partial fix for icc's buggy exception handling - at least print the error message */
#if __ICC==800
      fputs(errstring,stderr);
#endif 
#ifndef NDEBUG
      fputs(errstring,stderr);
      //      abort(); // do catch catch in gdb instead
#endif
    }
  public:
    const char* what() const throw() {return errstring;}
    error(const char *fmt,...)
    {
      va_list args;
      va_start(args, fmt);
      vsprintf(errstring,fmt,args);
      va_end(args);
      abort_if_debug();
    }
    error() {errstring[0]='\0'; abort_if_debug();}
    error(const error& e) {strncpy(errstring,e.errstring,200);}
  };
}
#endif
