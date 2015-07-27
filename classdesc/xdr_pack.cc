/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of Classdesc

  Open source licensed under the MIT license. See LICENSE for details.
*/

/*
  Move the XDR functionality into a separate compilation unit, as XDR
  has a very messy API
*/

#include "pack_base.h"
#include "classdesc_epilogue.h"
#ifdef XDR_PACK

#if (defined(_AIX) || defined(__osf__))
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
#endif

#if (defined(__ICC) && __ICC<900)
typedef long long int64_t;
#endif

extern "C"
{
#include <rpc/types.h>
#include <rpc/xdr.h>
}

// support for missing xdr prototypes (looking at you Macintosh!)
#ifdef NO_XDR_PROTO
extern "C"
{
  bool_t xdr_char(XDR *xdrs, char *cp);
  bool_t xdr_u_char(XDR *xdrs, unsigned char *ucp);
  bool_t xdr_int(XDR *xdrs, int *ip);
  bool_t xdr_u_int(XDR *xdrs, unsigned *up);
  bool_t xdr_short(XDR *xdrs, short *sp);
  bool_t xdr_u_short(XDR *xdrs, unsigned short *usp);
  bool_t xdr_long(XDR *xdrs, long *lp);
  bool_t xdr_u_long(XDR *xdrs, unsigned long *ulp);

#if HAVE_LONGLONG
  bool_t xdr_hyper(XDR *xdrs, long long *lp);
  bool_t xdr_u_hyper(XDR *xdrs, unsigned long long *lp);
#endif

  bool_t xdr_double(XDR *xdrs, double *dp);
  bool_t xdr_float(XDR *xdrs, float *fp);
  bool_t xdr_opaque(XDR *xdrs, char *cp, unsigned int cnt);

  void xdrmem_create(XDR *xdrs, char *addr, unsigned int size, enum xdr_op op);
  void xdrstdio_create(XDR *xdrs, FILE *file, enum xdr_op op); 
#ifndef xdr_destroy
  void xdr_destroy(XDR *xdrs);
#endif
#ifndef  xdr_getpos
  unsigned int xdr_getpos(XDR *xdrs);
#endif
#ifndef xdr_setpos
  void xdr_setpos(XDR *xdrs, unsigned int pos);
#endif
}
#endif

namespace classdesc
{
  struct XDR: public ::XDR {};


  template <>
  xdr_filter XDR_filter(const bool& x)               
  {return (xdr_filter) xdr_char;}

  template <>
  xdr_filter XDR_filter(const char& x)               
  {return (xdr_filter) xdr_char;}

  template <>
  xdr_filter XDR_filter(const signed char& x)        
  {return (xdr_filter) xdr_char;}

  template <>
  xdr_filter XDR_filter(const unsigned char& x)      
  {return (xdr_filter) xdr_u_char;}

  template <>
  xdr_filter XDR_filter(const wchar_t& x)            
  {return (xdr_filter) xdr_u_int;}

  template <>
  xdr_filter XDR_filter(const int& x)                
  {return (xdr_filter) xdr_int;}

  template <>
  xdr_filter XDR_filter(const unsigned int& x)       
  {return (xdr_filter) xdr_u_int;}

  template <>
  xdr_filter XDR_filter(const short int& x)          
  {return (xdr_filter) xdr_short;}

  template <>
  xdr_filter XDR_filter(const unsigned short int& x) 
  {return (xdr_filter) xdr_u_short;}

  template <>
  xdr_filter XDR_filter(const long int& x)           
  {return (xdr_filter) xdr_long;}

  template <>
  xdr_filter XDR_filter(const unsigned long int& x)  
  {return (xdr_filter) xdr_u_long;}

    /* long long is an extension to ANSI C++ that describes a 64 bit
       integer on all our supported compilers. XDR's terminolgy for
       this type is "hyper". If your compiler does not support 64 bit
       long longs, you may need to undefine HAVE_LONGLONG in your
       Makefile
     */

#ifdef HAVE_LONGLONG
  template <>
  xdr_filter XDR_filter(const long long& x)           
  {return (xdr_filter) xdr_hyper;}

  template <>
  xdr_filter XDR_filter(const unsigned long long& x)  
  {return (xdr_filter) xdr_u_hyper;}
#endif

  template <>
  xdr_filter XDR_filter(const float& x)              
  {return (xdr_filter) xdr_float;}

  template <>
  xdr_filter XDR_filter(const double& x)             
  {return (xdr_filter) xdr_double;}

  /* long doubles not supported in XDR */
  template <>
  xdr_filter XDR_filter(const long double& x)        
  {return NULL;}    

  xdr_pack::xdr_pack(size_t sz): pack_t(sz), asize(0)
  {
    input=new XDR; output=new XDR;
    xdrmem_create(input,(const caddr_t)data(),sz,XDR_ENCODE);
    xdrmem_create(output,(const caddr_t)data(),sz,XDR_DECODE);    
  }

  xdr_pack::xdr_pack(const char *fname, const char *rw):  pack_t(fname,rw), asize(0)
  {
    if (mode==writef)
      {
        input=new XDR; 
        output=NULL;
        xdrstdio_create(input,f,XDR_ENCODE);
      }
    else
      {
        input=NULL;
        output=new XDR;
        xdrstdio_create(output,f,XDR_DECODE);    
      }
  }

  xdr_pack::~xdr_pack() 
  {
    if (output && (mode==buf || mode==readf))
      {
        xdr_destroy(output); 
        delete output;
      }
    if (input && (mode==buf || mode==writef))
      {
        xdr_destroy(input); 
        delete input;  
      }
  }

  void  xdr_pack::append(const basic_type& x) 
    {
      if (mode==buf && BUFCHUNK-xdr_getpos(input)<8)
	{
	  asize += xdr_getpos(input);
	  realloc(asize+BUFCHUNK);
	  if (m_data==NULL) throw pack_error("Out of memory encoding XDR stream");
	  xdr_destroy(input); xdr_destroy(output);
	  xdrmem_create(input,(const caddr_t)data()+asize,BUFCHUNK,XDR_ENCODE);
	  xdrmem_create(output,(const caddr_t)data(),BUFCHUNK,XDR_DECODE);
	}
      if(!x.filter || !x.filter(input,x.val)) 
	throw pack_error("Error encoding XDR stream");
      m_size=asize+xdr_getpos(input);
    }
  
  void xdr_pack::popoff(basic_type& x)
  { 
    if (!x.filter || !x.filter(output,x.val)) 
      throw pack_error("Error encoding XDR stream");
    m_pos=xdr_getpos(output);
  }

  xdr_pack& xdr_pack::reseti() { 
    if (asize==0) 
      xdr_setpos(input,0);
    else
      {
	m_size=0; asize=0; xdr_destroy(input); 
	xdrmem_create(input,(const caddr_t)data(),BUFCHUNK,XDR_ENCODE);
      }
    return *this;
  }

  xdr_pack& xdr_pack::reseto() 
  { 
    xdr_setpos(output,0); 
    return *this; 
  }

  // assume that sizeof(size_t)==sizeof(long): true for most things
  xdr_pack& xdr_pack::seeki(long offs)
  { 
    assert(offs<=0);
    long seekpos=xdr_getpos(input)+offs;
    if (mode==buf && seekpos<0) 
      {
	seekpos+=asize;  /* work in absolute stream units, not relative to 
			    current segment */
	asize=seekpos/BUFCHUNK; seekpos%=BUFCHUNK;
	xdr_destroy(input);
	xdrmem_create(input,(const caddr_t)data()+asize,BUFCHUNK,XDR_ENCODE);
      }
    xdr_setpos(input,seekpos);
    m_size=asize+seekpos;
    return *this;
  }  

  xdr_pack& xdr_pack::seeko(long offs) 
  {
    xdr_setpos(output,xdr_getpos(output)+offs);
    return *this;
  }

  void xdr_pack::packraw(const char *x, size_t sz) 
  {
    if (mode==buf && BUFCHUNK-xdr_getpos(input)<sz)
      {
	asize += xdr_getpos(input);
	realloc(asize+BUFCHUNK);
	if (data()==NULL) throw pack_error("Error encoding XDR stream");
	xdr_destroy(input); xdr_destroy(output);
	xdrmem_create(input,(const caddr_t)data()+asize,BUFCHUNK,XDR_ENCODE);
	xdrmem_create(output,(const caddr_t)data(),BUFCHUNK,XDR_DECODE);
      }
    if (!xdr_opaque(input,const_cast<char*>(x),sz)) 
      throw pack_error("Error encoding XDR stream");
    m_size=asize+xdr_getpos(input);
  }

  void xdr_pack::unpackraw(char *x, size_t sz) 
  {
    if (!xdr_opaque(output,x,sz)) 
      throw pack_error("Error encoding XDR stream");
    m_pos=xdr_getpos(output);
  }

}
#endif // #ifdef XDR_PACK
