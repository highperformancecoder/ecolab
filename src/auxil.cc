/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include <eco_hashmap.h>
#include "ecolab_epilogue.h"

#if 0
char *index_name(int i)
{
  int j,k;
  tclindex idx;
  ostrstream iname;
  static char *iname_str=NULL;

  assert(dims.size>0 || dims.size==0 && i==0);
  if (dims.size==0) 
    iname << 0;
  else
    for (j=1, k=dims[0], iname << i%dims[0]; j<dims.size; k*=dims[j++])
      iname << ',' << (i/k)%dims[j];

  iname << ends;
  iname_str = (char*) realloc(iname_str,strlen(iname.str())+1);
  strcpy(iname_str,iname.str());
  return iname_str;
}

/* Which cell does species i correspond to? */
int which_cell(int i)
{
  int j, cell;
  tclindex idx;
  if (ncells==1) return 0;
  for ( cell=0, j=0; cell<ncells; cell++ )
    {
      j+=nsp[cell];
      if (i<j) return cell;
    }
  return cell;
} 
#endif

/* This code is a workaround for a bug conflict between gethostbyname
and libstdc++ on Linux systems. It only consults /etc/hosts. Feel free
to comment out if you have working gethostbyname already */

#if 0 /*__linux__ */
#include <netdb.h>
extern "C" struct hostent *gethostbyname(const char *name)
{
  static struct hostent r;
  static char *aliases[]={0};
  static char addr[]={127,0,0,1};
  static char *addresses[]={addr};
  FILE *hosts=fopen("/etc/hosts","r");
  char buffer[1024];
  while (fgets(buffer,1024,hosts)!=NULL)
    if (strstr(buffer,name)!=NULL)
      {
	for (int i=0; i<4; i++)
	  addr[i]=atoi(strtok( (i==0)?buffer: NULL, "."));
	printf("resolved %s as %d.%d.%d.%d\n",name,(unsigned char)addr[0],(unsigned char)addr[1],(unsigned char)addr[2],(unsigned char)addr[3]);
	break;
      }
  r.h_name=name;
  r.h_aliases=aliases;
  r.h_addrtype=2;
  r.h_length=4;
  r.h_addr_list=addresses;
  return &r;
}
#endif


#ifdef MEMDEBUG  /* use these new/delete replacements for tracking
down memory leaks */

#include <new>
#undef malloc
#undef free
int memused=0;


/* warning, warning! do not mix this Realloc with malloc and free! Use
   realloc(NULL,sz) as a replacement for malloc(sz), and
   realloc(ptr,0) as a replacement for free(ptr). */
#undef Realloc

namespace ecolab 
{

  // store size of allocation just prior to the returned pointer.
  char *Realloc(char *p, size_t sz)
  {
    char *r;
    if (sz==0)   /* equivalent to free(p); */
      r=NULL;
    else
      {
        // check whether we already have enough space
        if (p!=NULL && *reinterpret_cast<size_t*>(p-sizeof(size_t))>=sz) 
          return p; /* nothing needed */
        //      r = new char[sz];   /* new should not return NULL */
        r=(char*)malloc(sz+sizeof(size_t));
        *reinterpret_cast<size_t*>(r)=sz;
        r += sizeof(size_t);
        //      printf("%x allocated\n",r);
        if (p!=NULL) memcpy(r,p,sz);
      }
    memused += sz;
    if (p!=NULL) 
      {
        //      printf("%x deleted\n",p);
        p -= sizeof(size_t);
        memused -= *reinterpret_cast<size_t*>(p);
        free(p);
      }
    return r;
  }
}

#include <sys/types.h>
static int m=-1, maxm=0;

void *operator new(size_t s)
{
  char *r;
  //if (m++>maxm) {maxm=m; printf("alloc: %d\n",m);}  
  //printf("alloc: %d\n",m++);  
  // fake zero size allocations by adding 1
  r=ecolab::Realloc(NULL,s? s: 1); 
  if (memused>maxm)
    {
      printf("memused= %d Kbytes\n",memused/1024);
      maxm=memused;
    }
  //printf("new: %x\n",r);
  if (r==NULL) throw std::bad_alloc();
  assert(r!=NULL);
  return r;
}

void operator delete(void *p)
{
  //  if (p!=NULL) m--; //printf("delete: %d\n",m--);
  //if (p!=NULL) printf("delete: %d\n",m--);
  //if (p!=NULL) printf("delete: %x\n",p);
  //free(p);
  ecolab::Realloc((char*)p,0);
}

#endif

