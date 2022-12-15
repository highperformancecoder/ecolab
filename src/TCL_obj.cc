/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* support for TCL_obj class */
#include "TCL_obj_base.h"
#include "pack_base.h"
#include "ecolab_epilogue.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
using std::cout;
using std::cerr;
using std::endl;
using std::min;
using std::istringstream;
using std::ostringstream;
using namespace ecolab;

//#include <signal.h>
extern "C" 
{
  typedef void (*__sighandler_t) (int);
  extern __sighandler_t signal (int __sig, __sighandler_t __handler);
#define	SIGTERM		15	/* Termination (ANSI).  */
}
#include <unistd.h>

#ifdef MXE
#include <shlwapi.h>
#endif

namespace ecolab
{
#ifdef MXE
  static bool checkEcoLabLib(const string& x)
  {
    return PathFileExists((x+"/init.tcl").c_str());
  }

  static string dirName(const string& s)
  {
    string::size_type p=s.rfind('/');
    cout << "dirName "<<s<<" "<<p<<" "<<s.substr(0,p-1)<<endl;
    return p!=string::npos? s.substr(0,p-1): ".";
  }

  // With MXE builds, the ecolab_library refers to the host machine,
  // not the target, so we'll need to look in some standard places,
  // and leave it at that
  int setEcoLabLib(const char* path)
  {
    const char* cp;
    string s;
    
    if (((cp=getenv("ECOLAB_LIB")) && checkEcoLabLib(s=cp)) ||
        
        ((cp=getenv("HOME")) && checkEcoLabLib(s=string(cp)+"/usr/ecolab/include")) ||
        checkEcoLabLib(s="/usr/local/ecolab/include") ||
        ((cp=getenv("APPDATA")) && checkEcoLabLib(s=string(cp)+"/ecolab")) ||
        ((cp=Tcl_GetNameOfExecutable()) && checkEcoLabLib(s=dirName(cp)+"/library")))
      {
        Tcl_SetVar(ecolab::interp(),"ecolab_library",s.c_str(),TCL_GLOBAL_ONLY);
        Tcl_SetVar(ecolab::interp(),"tcl_library",(s+"/tcl").c_str(),TCL_GLOBAL_ONLY);
        Tcl_SetVar(ecolab::interp(),"tk_library",(s+"/tk").c_str(),TCL_GLOBAL_ONLY);
      }
    else
      cerr << "EcoLab runtime library not found.\n"
        "Please install one using (eg) the install.bat script from the EcoLab distribution,\n"
        "and/or specify an install location via the ECOLAB_LIB environment variable."<<endl;
    return 0;
  } 

#else
  int setEcoLabLib(const char* path)
  {
    Tcl_SetVar(ecolab::interp(),"ecolab_library",path,TCL_GLOBAL_ONLY);
    return 0;
  }
#endif
  
  bool interpExiting=false;
  void interpExitProc(ClientData cd)
  {
    interpExiting=true;
  }

  TCL_obj_t null_TCL_obj;
  
  int processEventsNest=0;
  NEWCMD(enableEventProcessing,0) {if (processEventsNest) processEventsNest--;}
  NEWCMD(disableEventProcessing,0) {processEventsNest++;}
  
  /* prototype command handler */

  template <class ARGS>
  int exec_cmd(cmd_data *cd, Tcl_Interp *interp, int argc, ARGS argv)
  {
    if (cd->nargs>=0 && cd->nargs>argc-1)
      {
        Tcl_AppendResult(interp,"Incorrect number of arguments",NULL);
        return TCL_ERROR;
      }
    /* process any pending events */
    while (processEventsNest==0 && Tcl_DoOneEvent(TCL_DONT_WAIT));  

#ifdef TIMECMDS
    tclvar cmd_times("cmd_times");
    clock_t t0=clock();
#endif

    //  if (myid==0)  puts(cd->name);   /* tracing of TCL commands */

    //no longer hard failing in DEBUG mode. Use the catch catch
    // command in gdb to debug exceptions being thrown.

    try {(cd->proc)(argc,argv);}
    catch (std::exception& e)
      {
        //      printf("exception caught %s\n",e.what());
        Tcl_AppendResult(interp,e.what(),NULL);
        return TCL_ERROR;
      }
    catch (...)
      {
        //      printf("undefined exception caught %s\n");
        Tcl_AppendResult(interp,"undefined exception caught",NULL);
        return TCL_ERROR;
      }
#ifdef TIMECMDS
  cmd_times[cd->name.c_str()] = 
    (exists(cmd_times[cd->name.c_str()])? 
     (double)cmd_times[cd->name.c_str()]: 0.0) +
    (double)(clock()-t0)/CLOCKS_PER_SEC;
#endif
    return TCL_OK;
  }

  int TCL_proc(ClientData cd, Tcl_Interp *interp, int argc, CONST84 char **argv)
  {return  exec_cmd((cmd_data *)cd, interp, argc, argv);}
  int TCL_oproc(ClientData cd, Tcl_Interp *interp, int argc, Tcl_Obj *const argv[])
  {return  exec_cmd((cmd_data *)cd, interp, argc, argv);}

  TCL_obj_hash& TCL_obj_properties()
  {
    static TCL_obj_hash data;
    return data;
  }

  void TCL_delete(ClientData cd)
  {
    /* find which entry corresponds to cd !! */
    TCL_obj_hash::iterator i;
    for (i=TCL_obj_properties().begin(); 
	 i->second.get()!=(member_entry_base*)cd && i!=TCL_obj_properties().end(); i++);
    if (i!=TCL_obj_properties().end()) 
      TCL_obj_properties().erase(i);
  }

  void TCL_obj_deregister(const string& desc )
  {
    /* delete all member commands associated with this */ 
    tclcmd c;
    int i, elemc; CONST84 char **elem;
    c | "info commands "|desc|".*\n";
    if (Tcl_SplitList(interp(),c.result.c_str(),&elemc,&elem)!=TCL_OK) 
      throw error("");
    for (i=0; i<elemc; i++) Tcl_DeleteCommand(interp(),elem[i]);  
    Tcl_DeleteCommand(interp(),desc.c_str());
    Tcl_Free((char*)elem);
    Tcl_SetResult(interp(),NULL,NULL);
  }

  const char* TCL_args::str() 
#if (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION==0)
    {return Tcl_GetStringFromObj(pop_arg(),NULL);}
#else
    {return Tcl_GetString(pop_arg());}
#endif

} //namespace ecolab

  /* client server code - not applicable for Windows */

#if !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(__MINGW32_VERSION)

#include <fcntl.h>

#ifdef ZLIB
#include <zlib.h>
#else
  typedef size_t uLongf; 
#endif
#include "Realloc.h"

  /* Field of the label specifying amount of data to be sent, including
     trailing NUL */
#define BUFSIZE_LABEL 32

  /* A pipe used to communicate between master and child process */
  static int dpipe[2]={0,0}, cpipe[2]; /* data pipe and control pipe */
  static Tcl_Channel sock;
static void close_socket(int d) {Tcl_Close(ecolab::interp(),sock); exit(0);}

  /* Utility routines for managing the pipe */
  /* nonblock=O_NONBLOCK or 0 */
  static int read_pipe(void *buffer,int size,int nonblock=0)
  {
    char ack;
    fcntl(dpipe[0],F_SETFL,nonblock);
    int s;
  
    for (s=0; s<size;)
      {
        s+=read(dpipe[0],((char*)buffer)+s,size-s);
        if (s<0) return s;
        if (write(cpipe[1],&ack,1)==-1)
          throw error(strerror(errno));
      }
    return s;
  }

  //This constant is not always defined...
#ifndef PIPE_BUF
#define PIPE_BUF 1024
#endif

  static void write_pipe(void *buffer,int size)
  {
    char ack;
    for (int s=0; s<size; s+=PIPE_BUF)
      {
        if (write(dpipe[1],((char*)buffer)+s,min(size-s,PIPE_BUF))==-1 ||
            read(cpipe[0],&ack,1)==-1)
          throw error(strerror(errno));
      }
  }

/* get global variables from server at argv[1] at port argv[2] */

void classdesc::TCL_obj_t::get_vars(int argc, char *argv[])
{
  static int childpid=0;
  
  if (argc<3) throw error("not enough arguments to %s\n",argv[0]);
  
  if (childpid)   /* handler already running */
    {
      size_t sz;
      if (read_pipe(&sz,sizeof(int),O_NONBLOCK)==-1) return; /* no data yet */
      xdr_pack buffer(sz);
      sz=read_pipe(buffer.data(),sz);
      if (sz != buffer.size())
        throw error("%d bytes received, %d expected",sz,int(buffer.size()));
      check_functor->unpack(buffer);
      ecolab::tclvar model_valid("model_valid"); model_valid=1;
      return;
    }

  if (dpipe[0]==0 && (pipe(dpipe)||pipe(cpipe))) throw error("Error creating pipe");

  if (!(childpid=fork()))
    {
      unsigned long sz, bufsz;
      int zret;
      uLongf ubufsz;
      char *buffer=NULL, *ubuffer=NULL;
      char bufsize[BUFSIZE_LABEL];
      close(dpipe[0]); 
      close(cpipe[1]); 

      signal(SIGTERM,close_socket);

      for (;;)
        {
          sock=Tcl_OpenTcpClient(interp(),atoi(argv[2]),argv[1],0,0,0);
          Tcl_SetChannelOption(interp(),sock,"-translation","binary");
          Tcl_Read(sock,bufsize,BUFSIZE_LABEL);

          //sscanf(bufsize,"%ld %lud",&ubufsz,&sz);
          istringstream bufszs(bufsize);
          bufszs>>ubufsz>>sz;
          buffer=(char*)realloc(buffer,sz);	  
          ubuffer=(char*)realloc(ubuffer,ubufsz);	  
          for (bufsz=0; bufsz<sz; 
               bufsz += Tcl_Read(sock,buffer+bufsz,sz-bufsz));  

          if (sz<ubufsz)   /* data is compressed */
            {
#ifdef ZLIB
              if (Z_OK!=(zret=uncompress((Bytef*)ubuffer,&ubufsz,
                                       (Bytef*)buffer,bufsz)))
                {
                  printf("Compression failure: %d\n",zret);
                  goto abort;
                }
              else
                {char *tmp; tmp=buffer; buffer=ubuffer; ubuffer=tmp; sz=ubufsz;}
#else
          /* we're sunk */
              printf("zlib not available\n");
#endif
            }
          write_pipe(&sz,sizeof(sz));
          write_pipe(buffer,sz);
        abort:
          Tcl_Close(interp(),sock);
        }
    }
  else
    {close(dpipe[1]); close(cpipe[0]);}
}


static void dumpdata(ClientData cd, Tcl_Channel channel, char* host, int port)
{
  classdesc::xdr_pack buffer;
  char bufsize[BUFSIZE_LABEL], *sendbuf;
  static char *compbuf=NULL;
  ((classdesc::TCL_obj_t*)cd)->check_functor->pack(buffer);

#ifdef ZLIB
  uLongf compsz=13+1.1*buffer.size();
  compbuf=(char*)classdesc::realloc(compbuf,compsz);
  if (Z_OK!=compress((Bytef*)compbuf,&compsz,(Bytef*)buffer.data(),buffer.size()))
    compsz=buffer.size();
#else
  size_t compsz=buffer.size();
#endif

  if (compsz<buffer.size())
    sendbuf=compbuf;
  else
    {sendbuf=buffer.data(); compsz=buffer.size();}

  sprintf(bufsize,"%*ld %*ld\n",BUFSIZE_LABEL/2-2,long(buffer.size()),BUFSIZE_LABEL/2-2,
          long(compsz));
  Tcl_SetChannelOption(interp(),channel,"-translation","binary");
  Tcl_Write(channel,bufsize,BUFSIZE_LABEL);
  Tcl_Write(channel,sendbuf,compsz);
  Tcl_Close(interp(),channel);
}

  /* set up a data server on port argv[1] to send global variables to an
     ecolab frontend */
void classdesc::TCL_obj_t::data_server(int argc, char* argv[])
{
  if (!Tcl_OpenTcpServer(interp(),atoi(argv[1]),NULL,dumpdata,(ClientData)this))
    perror("Error in data_server");
}


#else


void classdesc::TCL_obj_t::get_vars(int argc, char *argv[])
{
  throw error("get_vars not implemented on Win32");
}


void classdesc::TCL_obj_t::data_server(int argc, char* argv[])
{
  throw error("data_server not implemented on Win32");
}


#endif

  /*
    create a temporary name in current directory
  */
static std::string tmpname(const std::string prefix)
  {
    for (int i=0;; ++i)
      {
        ostringstream t;
        struct stat buf;
        t<<prefix<<i;
        if (stat(t.str().c_str(), &buf))
          {
            if (errno!=ENOENT)
              throw error("tmpname error: %s",strerror(errno));
            else 
              return t.str();
          }
      }
  }

void classdesc::TCL_obj_t::checkpoint(int argc, char *argv[])  
{
  if (argc<2) throw error("not enough arguments to %s",argv[0]);
  std::string tmp = tmpname(argv[1]);
  if (xdr_check)
    {
      xdr_pack buf(tmp.c_str(),"w");
      check_functor->pack(buf);
    }
  else
    {
      pack_t buf(tmp.c_str(),"w");
      check_functor->pack(buf);
    }
  //now that the checkpoint is on disk, replace any existing
  //checkpoint file with the new one
  rename(tmp.c_str(),argv[1]);
}

  void classdesc::TCL_obj_t::restart(int argc, char *argv[]) 
  {
    if (argc<2) throw error("not enough arguments to %s",argv[0]);
    if (xdr_check)
      {
        xdr_pack buf(argv[1],"r");
        check_functor->unpack(buf);
      }
    else
      {
        pack_t buf(argv[1],"r");
        check_functor->unpack(buf);
      }
  }

