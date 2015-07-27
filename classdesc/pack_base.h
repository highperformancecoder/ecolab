/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
   \brief serialisation descriptor
*/

#ifndef PACK_BASE_H
#define PACK_BASE_H
#include "classdesc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <assert.h>
#include <stdarg.h>
#include <exception>
#include <vector>
#include <algorithm>
#include <typeinfo>

#include <Realloc.h>

#ifdef _MSC_VER
// stupid warning about unassignable objects
#pragma warning(disable:4512)
#endif

namespace classdesc
{
  struct XDR;

  class pack_error : public exception 
  {
    string msg;
  public:
    pack_error(const char *s): msg("pack:") {msg+=s;}
    virtual ~pack_error() throw() {}
    virtual const char* what() const throw() {return msg.c_str();}
  };

#ifdef XDR_PACK
  typedef bool (*xdr_filter)(XDR*,...);
#endif

  struct basic_type
  {
    void *val;
    size_t size;
#ifdef XDR_PACK
    xdr_filter filter;
#endif
  };

#ifdef XDR_PACK
  template <class T> xdr_filter XDR_filter(const T&);
#endif
 
  template <class T>
  struct Basic_Type: public basic_type
  {
    Basic_Type(const T& x){
      val=(void*)&x; size=sizeof(x);
#ifdef XDR_PACK
      filter=XDR_filter(x);
#endif      
    }
  };

  /**
     What to do with pointers: throw an exception, use graphnode or treenode 
     algorithm)
  */

  enum Ptr_flag {DEFAULT, GRAPH, TREE}; 

  /**
     class for handling data allocated on heap by graphnode and treenode 
     algorithms
  */
  
  struct PtrStoreBase
  {
    int cnt;
    PtrStoreBase(): cnt(0) {}
    virtual ~PtrStoreBase() {};
    virtual void *data()=0;
  };

  /**
     class for handling data allocated on heap by graphnode and treenode 
     algorithms
  */
  template <class T>
  struct PtrStore: public PtrStoreBase
  {
    T d;
    void* data() {return &d;}
  };

  /**
     class for handling data allocated on heap by graphnode and treenode 
     algorithms
  */
  class PtrStoreRef
  {
    PtrStoreBase *d;
  public:
    void* data() {return d->data();}
    PtrStoreRef(PtrStoreBase *d=NULL): d(d) {}
    PtrStoreRef(const PtrStoreRef& x): d(x.d) {d && d->cnt++;}
    PtrStoreRef& operator=(const PtrStoreRef& x) {d=x.d; d && d->cnt++; return *this;}
    ~PtrStoreRef() {if (d && d->cnt-- == 0) delete d;}
  };

  /**
     basic serialisation buffer object
  */
  class pack_t
  {
#if __cplusplus < 201103L
    pack_t operator=(const pack_t&){return *this;}
    pack_t(const pack_t&){}
#endif
  protected:
    FILE *f;
    void swap_base (pack_t& other){
      std::swap(f,other.f);
      std::swap(mode,other.mode);
      std::swap(m_data,other.m_data);
      std::swap(m_size,other.m_size);
      std::swap(m_pos,other.m_pos);
      std::swap(ptr_flag,other.ptr_flag);
      std::swap(alloced,other.alloced);
    }
    enum mode_t {buf, readf, writef};
    mode_t mode;
    char *m_data; ///< actual buffer
    size_t m_size;  ///< size of buffer 
    size_t m_pos;   ///< position of read pointer
  public:
    //  class notimplemented{};
    //  int xdr;
    Ptr_flag ptr_flag;
    unsigned recur_max; ///< recursion limit for pack_graph
    std::vector<PtrStoreRef> alloced; //allocated data used for cleaning up 
    const char* data() const {return m_data;}  ///< actual buffer
    char* data() {return m_data;}  ///< actual buffer
    size_t size() const {return m_size;}  ///< size of buffer
    size_t pos() const {return m_pos;}  ///< position of read pointer
    char* realloc(char* d, size_t s) {
#ifdef Realloc
      return (char*)Realloc(d,s);
#else
      return (char*)classdesc::realloc(d,s);
#endif
    }
    void realloc(size_t s) {m_data=realloc(m_data,s);}
     
    virtual void append(const basic_type& x)
    {
      if (mode==buf)
        {
          realloc(m_size+x.size);
          if (!m_data)
            throw std::bad_alloc();
          memcpy(m_data+m_size,x.val,x.size);
        }
      else
        if (fwrite(x.val,x.size,1,f) != 1)
          throw pack_error("file write fail");
      m_size+=x.size;
    }
    virtual void popoff(basic_type& x)
    {
      if (mode==buf)
        if (m_pos+x.size>m_size)
          throw pack_error("unexpected end of data");
        else
          memcpy(x.val,m_data+m_pos,x.size);
      else
        if (fread(x.val,x.size,1,f) != 1)
          throw pack_error("unexpected end of file");
      m_pos+=x.size;
    }

    pack_t(size_t sz=0): f(0), mode(buf), m_data(NULL), m_size(sz), m_pos(0), ptr_flag(DEFAULT), recur_max(500) {
#ifdef RECUR_MAX
      recur_max=RECUR_MAX;
#endif
      realloc(sz);}
    pack_t(const char *fname, const char *rw): 
      f(fopen(fname,rw)), mode( (rw&&rw[0]=='w')? writef: readf), 
      m_data(0), m_size(0), m_pos(0), ptr_flag(DEFAULT), recur_max(500)  {
      // this is to support a deprecated interface
#ifdef RECUR_MAX
      recur_max=RECUR_MAX;
#endif
      if (!f) throw pack_error(strerror(errno));
    }
    virtual ~pack_t() {realloc(0); if (f) fclose(f);}

#if __cplusplus >= 201103L
    pack_t(pack_t&& x): f(nullptr), m_data(nullptr) {swap(x);}
    pack_t& operator=(pack_t&& x) {swap(x); return *this;}
#endif

    virtual operator bool() {return m_pos<m_size;}
    virtual pack_t& reseti() {m_size=0; if (f) fseek(f,0,SEEK_SET); return *this;}
    virtual pack_t& reseto() {m_pos=0; if (f) fseek(f,0,SEEK_SET); return *this;}
    virtual pack_t& seeki(long offs) {
      assert(offs<=0); m_size+=offs; 
      if (f) fseek(f,offs,SEEK_CUR); 
      return *this;
    }
    virtual pack_t& seeko(long offs) {
      m_pos+=offs;       
      if (f) fseek(f,offs,SEEK_CUR); 
      return *this;
    }
    void clear() {realloc(0); m_data=0; m_size=m_pos=0;}
    virtual void packraw(const char *x, size_t s) 
    {
      if (mode==buf)
        {
          realloc(m_size+s); 
          memcpy(m_data+m_size,x,s); m_size+=s;
        }
      else
        if (fwrite(x,s,1,f)!=1)
          throw pack_error("filed to write data to stream");
          
    }
    virtual void unpackraw(char *x, size_t s) 
    {
      if (mode==buf)
        memcpy(x,m_data+m_pos,s);
      else
        if (fread(x,s,1,f)!=1)
          throw pack_error("premature end of stream");
      m_pos+=s;
    }
    virtual void swap(pack_t& other) {
      if (typeid(*this)!=typeid(other))
        throw pack_error("cannot swap differing types");
      swap_base(other);
    }
    /// returns -1, 0 or 1 if this is lexicographically less, equal or
    /// greater than \a x
    virtual int cmp(const pack_t& x) const {
      if (m_size==x.m_size)
        return memcmp(m_data,x.m_data,m_size);
      else
        return m_size<x.m_size? -1: 1;
    }

    bool operator<(const pack_t& x) const {return cmp(x)==-1;}
    bool operator>(const pack_t& x) const {return cmp(x)==1;}
    bool operator==(const pack_t& x) const {return cmp(x)==0;}
    bool operator!=(const pack_t& x) const {return cmp(x)!=0;}
  };

  /// deep comparison of two serialisable items
  template <class T>
  int deepCmp(const T& x, const T& y) {
    pack_t xb, yb;
    return (xb<<x).cmp(yb<<y);
  }
  /// deep equality of two serialisable items
  template <class T>
  bool deepEq(const T& x, const T& y) {
    pack_t xb, yb;
    return (xb<<x).cmp(yb<<y)==0;
  }

  typedef pack_t unpack_t;

#ifdef XDR_PACK
  const int BUFCHUNK=1024;  
  
  /**
     machine independent serialisation buffer object
  */
  class xdr_pack: public pack_t
  {
    size_t asize;
    XDR *input, *output;
  public:
    xdr_pack(size_t sz=BUFCHUNK);
    xdr_pack(const char *, const char* rw);
    ~xdr_pack();
#if __cplusplus >= 201103L
    xdr_pack(xdr_pack&& x): input(nullptr), output(nullptr) {swap(x);}
    xdr_pack& operator=(xdr_pack&& x) {swap(x); return *this;}
#endif

    virtual void append(const basic_type& x); 
    virtual void popoff(basic_type& x);
    virtual xdr_pack& reseti();
    virtual xdr_pack& reseto();
    virtual xdr_pack& seeki(long offs);
    virtual xdr_pack& seeko(long offs);
    virtual void packraw(const char *x, size_t sz);
    virtual void unpackraw(char *x, size_t sz);
    virtual void swap(pack_t& other) {
      xdr_pack* xdr_other=dynamic_cast<xdr_pack*>(&other);
      if (!xdr_other) throw pack_error("cannot swap differing types");
      swap_base(other);
      std::swap(asize,xdr_other->asize);
      std::swap(input,xdr_other->input);
      std::swap(input,xdr_other->output);
    }
  };
#else
  typedef pack_t xdr_pack;
#endif

  /** Binary streamer class. This subverts the normal classdesc
      serialisation to just stream types directly to a pack_t
      type, for efficiency. Only useful for PODs. 

      Caveat: Do not use on classes with virtual functions!
  */

  class BinStream
  {
    pack_t& packer;
  public:
    BinStream(pack_t& packer): packer(packer) {}
    template <class T> 
    typename enable_if<Not<is_container<T> >, BinStream&>::T
    operator<<(const T& o)
    {
      packer.packraw(reinterpret_cast<const char*>(&o), sizeof(T)); 
      return *this;
    }
    template <class T> 
    typename enable_if<Not<is_container<T> >, BinStream&>::T
    operator>>(T& o)
    {
      packer.unpackraw(reinterpret_cast<char*>(&o), sizeof(T)); 
      return *this;
    }

    template <class T> 
    typename enable_if<is_container<T>, BinStream&>::T
    operator<<(const T& o)
    {
      (*this)<<o.size();
      for (typename T::const_iterator i=o.begin(); i!=o.end(); ++i)
        (*this)<<*i;
      return *this;
    }

    template <class T> 
    typename enable_if<is_sequence<T>, BinStream&>::T
    operator>>(T& o)
    {
      size_t s;
      (*this)>>s;
      typename T::value_type v;
      for (size_t i=0; i<s; ++i)
        {
          (*this)>>v;
          o.push_back(v);
        }
      return *this;
    }

    template <class T> 
    typename enable_if<is_associative_container<T>, BinStream&>::T
    operator>>(T& o)
    {
      size_t s;
      (*this)>>s;
      typename T::value_type v;
      for (size_t i=0; i<s; ++i)
        {
          (*this)>>v;
          o.insert(v);
        }
      return *this;
    }

    /// specialisation for vector
    template <class T, class A> 
    inline BinStream& operator<<(const std::vector<T,A>& o);

    template <class T, class A> 
    inline BinStream& operator>>(std::vector<T,A>& o);
    
  };

  /**
     convenience Template for creating a binary streamer in one hit
     BinStreamT cannot be a Pack, as this causes ambiguity in the
     resolution of the streaming operators. So instead it has one.
  */
  template <class Pack> struct BinStreamT: public BinStream 
  {
    Pack thePack;
    BinStreamT(): BinStream(thePack) {}
    template <class A1> BinStreamT(A1 a1):
      BinStream(thePack), thePack(a1)  {}
    template <class A1, class A2> BinStreamT(A1 a1, A2 a2):
      BinStream(thePack), thePack(a1, a2)  {}
  };
     

  /**
     Used to indicate a member is not to be serialised: eg
     class foo
     {
     unserialisable<std::vector<int>::iterator> i;
     ...
     };
     Deprecated in favour of Exclude
  */
  template <class T>
  struct unserialisable: public T 
  {
    unserialisable() {}
    unserialisable(const T& x): T(x) {}
    unserialisable operator=(const T& x) {T::operator=(x); return *this;}
  };


  template <class T>  pack_t& operator<<(pack_t& y,const T&x);
  template <class T>  pack_t& operator>>(pack_t& y,T&x);

  /// for use in metaprogramming support. Indicate that a given type
  /// is supported explicitly
  template <class T> struct pack_supported: 
    public is_fundamental<T> {};

#ifndef THROW_PTR_EXCEPTION
  template <class T>
  inline void pack(pack_t& targ, const string& desc, is_treenode dum, const T* const& arg);

  template <class T>
  inline void unpack(unpack_t& targ, const string& desc, 
                     is_treenode dum, T*& arg);

  template <class T>
  inline void pack(pack_t& targ, const string& desc,
                   is_graphnode dum,const T& arg);

  ///unserialise a graph structure
  /** can contain cycles */
  template <class T>
  inline void unpack(pack_t& targ, const string& desc,
                     is_graphnode dum,T& arg);

#endif

  template <class T>
  void pack_onbase(pack_t& x,const string& d,T& a)
  {pack(x,d,a);}

  template <class T>
  void unpack_onbase(unpack_t& x,const string& d,T& a)
  {unpack(x,d,a);}
}

using classdesc::pack_onbase;
using classdesc::unpack_onbase;

/**
   @namespace classdesc_access \brief Contains access_* structs, and nothing
   else. These structs are used to gain access to private members
*/

namespace classdesc_access
{
  /// class to allow access to private members
  template <class T> struct access_pack;
  /// class to allow access to private members
  template <class T> struct access_unpack;

  /* default action for pointers is to throw an error message */
  template <class T>
  struct access_pack<T*>
  {
    template <class C>
    void operator()(classdesc::pack_t& targ, const classdesc::string& desc, C& arg)
    {
      switch (targ.ptr_flag)
        {
          // You may wish to define this macro if you have messy types containing pointers
#ifndef THROW_PTR_EXCEPTION
        case classdesc::GRAPH: 
          pack(targ,desc,classdesc::is_graphnode(),arg);
          break;
        case classdesc::TREE: 
          pack(targ,desc,classdesc::is_treenode(),arg);
          break;
#endif
        default:
          throw classdesc::pack_error("Packing arbitrary pointer data not implemented");
        }
    }
  };

  template <class T>
  struct access_unpack<T*>
  {
    template <class C>
    void operator()(classdesc::unpack_t& targ, const classdesc::string& desc, C& arg)
    {
      switch (targ.ptr_flag)
        {
          // You may wish to define this macro if you have messy types containing pointers
#ifndef THROW_PTR_EXCEPTION
        case classdesc::GRAPH: 
          unpack(targ,desc,classdesc::is_graphnode(),arg);
          break;
        case classdesc::TREE: 
          unpack(targ,desc,classdesc::is_treenode(),arg);
          break;
#endif
        default:
          throw classdesc::pack_error("Unpacking arbitrary pointer data not implemented");
        }
    }
  };

  template <class T>
  struct access_pack<classdesc::unserialisable<T> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class T>
  struct access_unpack<classdesc::unserialisable<T> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class T>
  struct access_pack<classdesc::Exclude<T> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};

  template <class T>
  struct access_unpack<classdesc::Exclude<T> >:
    public classdesc::NullDescriptor<classdesc::pack_t> {};

}

namespace classdesc
{
  //generic pack, unpack - defined in pack_stream
  template <class T> typename
  enable_if<Not<pack_supported<T> >, void>::T
  pack(pack_t& buf, const string& desc, T& arg)
  {classdesc_access::access_pack<T>()(buf,desc,arg);}
  

  template <class T> typename 
  enable_if<Not<pack_supported<T> >, void>::T
  unpack(unpack_t& buf, const string& desc, T& arg)
  {classdesc_access::access_unpack<T>()(buf,desc,arg);}

  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  pack(pack_t& targ, const string&, T& arg)
  {
    Basic_Type<T> b(arg);
    targ.append(b);
  }

  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  pack(pack_t& targ, const string&, const T& arg)
  {
    Basic_Type<T> b(arg);
    targ.append(b);
  }

  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  unpack(unpack_t& targ, const string&, T& arg)
  {
    Basic_Type<T> b(arg);
    targ.popoff(b);
  }

  template <class T>
  typename enable_if<is_fundamental<T>, void>::T
  unpack(unpack_t& targ, const string&, const T&)
  { /* const vars cannot be unpacked */
    T dum; Basic_Type<T> b(dum);
    targ.popoff(b);
  }
  

  /* The problem is that function pointers this template also, so we
     need a separate pack to packup pointers to single objects:

     pack a flag indicating validity, and then the object pointer points
     to. Cannot handle arrays, but useful for graphs - assumes pointer is
     valid unless NULL.

  */


  template <class T>
  void pack(pack_t& targ, const string& desc, is_array, 
            T &arg,int dims,size_t ncopies,...)
  {
    va_list ap;
    va_start(ap,ncopies);
    for (int i=1; i<dims; i++) ncopies*=va_arg(ap,int); //assume that 2 and higher D arrays dimensions are int
    va_end(ap);
    for (size_t i=0; i<ncopies; i++) pack(targ,desc,(&arg)[i]);
  }

  template <class T>
  void unpack(unpack_t& targ, const string& desc, is_array, T &arg,
              int dims,size_t ncopies,...)
  {
    va_list ap;
    va_start(ap,ncopies);
    for (int i=1; i<dims; i++) ncopies*=va_arg(ap,int);
    va_end(ap);
    for (size_t i=0; i<ncopies; i++) unpack(targ,desc,(&arg)[i]);
  }

  /* specialise for char* */
  inline void pack(pack_t& targ, const string&, is_array, 
                   char &arg,int dims,size_t ncopies,...)
  {
    int i;
    va_list ap;
    va_start(ap,ncopies);
    for (i=1; i<dims; i++) ncopies*=va_arg(ap,int);
    va_end(ap);
    targ.packraw(&arg,ncopies);
  }

  inline void unpack(unpack_t& targ, const string&, is_array, 
                     char &arg,int dims,size_t ncopies,...)
  {
    int i;
    va_list ap;
    va_start(ap,ncopies);
    for (i=1; i<dims; i++) ncopies*=va_arg(ap,int);
    va_end(ap);
    targ.unpackraw(&arg,ncopies);
  }

  /* what to do about member functions */

  template<class C, class T>
  void pack(pack_t&, const string&, C&, T) {} 
  
  template<class C, class T>
  void unpack(unpack_t&, const string&, C&,  T) {} 
    
  /*
    Method pointer serialisation
  */
  template <class C, class R, class A1>
  void pack(pack_t& targ, const string& desc, R (C::*&arg)(A1))
  {targ.packraw((char*)&arg,sizeof(arg));}

  template <class C, class R, class A1>
  void unpack(pack_t& targ, const string& desc, R (C::*&arg)(A1))
  {targ.unpackraw((char*)&arg,sizeof(arg));}

  /// const static support. No need to stream
  template <class T>
  void pack(pack_t& targ, const string& desc, is_const_static i, T t)
  {}

  template <class T>
  void unpack(pack_t& targ, const string& desc,is_const_static i, T t)
  {}

  // static methods
  template <class T, class U>
  void pack(pack_t&, const string&,is_const_static, const T&, U) {}

  template <class T, class U>
  void unpack(pack_t& targ, const string& desc,is_const_static i, const T&, U) {}

  // to handle pack/unpacking of enums when -typeName is in effect
  template <class E>
  void pack(pack_t& targ, const string& desc,Enum_handle<E> a) 
  {
    int x=a;
    pack(targ,desc,x);
  }

  template <class E>
  void unpack(pack_t& targ, const string& desc, Enum_handle<E> a) 
  {
    int x;
    unpack(targ,desc,x);
    a=x;
  }

}

using classdesc::pack;
using classdesc::unpack;
using classdesc::pack_onbase;
using classdesc::unpack_onbase;

#include "pack_stl.h"

#ifdef _CLASSDESC
#pragma omit pack classdesc::string
#pragma omit pack eco_strstream
#pragma omit pack xdr_pack
#pragma omit unpack classdesc::string
#pragma omit unpack eco_strstream
#pragma omit unpack xdr_pack
#endif
#endif
