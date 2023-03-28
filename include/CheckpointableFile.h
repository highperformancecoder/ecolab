/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief checkpointable binary file writer
*/
#ifndef CHECKPOINTABLE_FILE_H
#define CHECKPOINTABLE_FILE_H

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "error.h"
#include "pack_base.h"
#include "pack_stl.h"

namespace ecolab
{

  /// checkpointable binary file writer
  /**
     A Checkpointable file is one that saves its current cursor position
     and filename when serialised, so that when restored, writes will
     continue from where it left off. Only write operations are
     supported on Checkpointable_file.
  */
  class Checkpointable_file
  {
    // we use std streams because we can get an integral position type
    std::FILE *f; 
    std::string name;
    void operator=(const Checkpointable_file&);
    Checkpointable_file(const Checkpointable_file&);
  public:
    Checkpointable_file(): f(NULL) {}
    Checkpointable_file(const std::string& name) {open(name);}
    ~Checkpointable_file() {close();}

    /// open file \a n in mode \a mode (taking modes acceptible to fopen)
    void open(const std::string& n, const char* mode="w") {
      f=fopen(n.c_str(),mode);
      if (!f) throw error("error opening %s: %s",n.c_str(),strerror(errno));
      name=n;
    }
    void close() {fclose(f); name="";}
    bool opened() const {return f;}

    /// low level write call - like std::fwrite
    void write(const void*buf, std::size_t size, std::size_t nobj) {
      if (!f) throw error("file %s not open",name.c_str());
      std::size_t obj_written=fwrite(buf,size,nobj,f);
      if (obj_written!=nobj)
        throw error("written only %d of %d objects: %s",
                    obj_written,nobj,strerror(errno));
    }

    /// write a vector of data
    template <class T, class A>
    void write(const std::vector<T,A>& buf)
    {if (!buf.empty()) write(buf.data(),sizeof(T),buf.size());}

    /// stream data as text like std::iostream
    template <class T>
    Checkpointable_file& operator<<(const T& x) {
      std::ostringstream o; 
      o<<x;
      write(o.str().c_str(),1,o.str().length());
      return *this;
    }

    void pack(classdesc::pack_t& buf) const {
      buf<<opened();
      if (opened())
        {
          if (fflush(f))
            throw error("flush failure: %s",strerror(errno));
          long p=ftell(f); buf<<p; // 32bit warning!
          if (p==-1L) throw error("ftell error: %s",strerror(errno));
          buf<<name;
        }
    }
    void unpack(classdesc::pack_t& buf) {
      if (opened()) close();
      bool is_open; buf>>is_open;
      if (is_open) {
        long p; buf>>p;
        buf>>name;
        open(name,"r+");
        if (fseek(f,p,SEEK_SET))
          throw error("fseek error: %s",strerror(errno));
      }
    }
  };

}

namespace classdesc_access
{
  template <> struct access_pack<ecolab::Checkpointable_file>
  {
    template <class U>
    void operator()(classdesc::pack_t& b, const classdesc::string& d, U& a) 
    {a.pack(b);}
  };

  template <> struct access_unpack<ecolab::Checkpointable_file>
  {
    template <class U>
    void operator()(classdesc::pack_t& b, const classdesc::string& d, U& a) 
    {a.unpack(b);}
  };
}

#endif
