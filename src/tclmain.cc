/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "ecolab.h"
#include "object.h"
#include "eco_hashmap.h"
#include "cairoSurfaceImage.h"
#include "ecolab_epilogue.h"
//#include <signal.h>
extern "C" 
{
  typedef void (*__sighandler_t) (int);
  extern __sighandler_t signal (int __sig, __sighandler_t __handler);
}
#define SIG_DFL	((__sighandler_t) 0)		/* Default action.  */
#define	SIGILL		4	/* Illegal instruction (ANSI).  */
#define	SIGABRT		6	/* Abort (ANSI).  */
#define	SIGBUS		7	/* BUS error (4.2 BSD).  */
#define	SIGSEGV		11	/* Segmentation violation (ANSI).  */


using namespace std;
using namespace ecolab;
using namespace classdesc;

#include "version.h"
NEWCMD(ecolab_version,0)
{
  tclreturn r;
  r<<VERSION;
}

namespace ecolab
{
  unsigned myid=0, nprocs=1;   /* MPI task ID and no. of threads*/
#ifdef TK
  Tk_Window mainWin=0;
#endif
}

#if MPI_SUPPORT
#include <mpi.h>

const int MPI_error_tag=1000; /* hopefully users won't use this? */

NEWCMD(handle_error_return,0)
{
  tclcmd c;
  MPIbuf b;
  c << "after 1000 handle_error_return\n"; /* check every second */
  if (b.msg_waiting(MPI_ANY_SOURCE,MPI_error_tag))
    {
      b.get();
      throw error("\nMPI Error on %d: %s",b.proc,b.data());
    }
}

void return_error(const char* s)
{
  MPIbuf b;
  b.packraw(s,strlen(s)+1);
  b.send(0,MPI_error_tag);
}

/* MPI slave command loop */
void do_slave_loop()
{
  /* we have to assume a maximum size of message passed through :( */
  tclcmd cmd;
  string c;
  for (;;)  /* the only way out of this loop is to send the quit command */
    {
      MPIbuf().bcast(0) >> c;
      try {cmd << c << '\n';}
      catch (std::exception& e) {return_error(e.what());}
      catch (...) {return_error("unknown execption caught");}
    }
}

/* force MPI_Finalize to be called at terminate time */
struct MPI_init
{
  MPI_init(int &c, char**& v) {MPI_Init(&c,&v);}
  ~MPI_init() 
  {
    if (myid==0)
      {
	parsend("finalize");
	MPI_Finalize();
      }
  }
};

NEWCMD(finalize,0)
{MPI_Finalize(); exit(0);}

void throw_MPI_errors(MPI_Comm * c, int *code, ...)
{
  char *errstr=new char[MPI_MAX_ERROR_STRING+1];
  int length;
  MPI_Error_string(*code,errstr,&length);
  errstr[length]='\0';
  error MPI_err(errstr);
  delete [] errstr;
  throw MPI_err;
}

#endif

#ifdef MEMDEBUG
extern int memused;
struct CheckMemusedAtExit
{
  ~CheckMemusedAtExit() {printf("%d bytes remaining to be freed\n", memused);}
};
#endif

#include "timer.h"
#ifdef TIMER
struct MainTimer
{
  MainTimer() {start_timer("main");}
  ~MainTimer() {
    stop_timer("main");
    print_timers();
  }
};
#else
struct MainTimer {};
#endif

int main(int argc, char* argv[])
{
  MainTimer mainTimer; // start a timer of everything, and print results on exit

  Tcl_FindExecutable(argv[0]);
  Tcl_Init(interp());
#ifdef MPI_SUPPORT
  MPI_init foo(argc,argv);	  
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD,(int*)&myid);
  MPI_Comm_size(MPI_COMM_WORLD,(int*)&nprocs);
  /* config MPI to throw an error() */
  MPI_Errhandler errhandler;
#if MPI_VERSION>=2
  MPI_Comm_create_errhandler((MPI_Comm_errhandler_function*)throw_MPI_errors,&errhandler);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD,errhandler);
#else
  MPI_Errhandler_create((MPI_Handler_function*)throw_MPI_errors,&errhandler);
  MPI_Errhandler_set(MPI_COMM_WORLD,errhandler);
#endif
#endif

  /* set the TCL variables argc and argv to contain the
     arguments. argv[0] will contain the script name, not the
     interpreter name */
  tclvar tcl_argc("argc"), tcl_argv("argv");
  tcl_argc=argc-1;
  if (argc>1)
    for (int i=1; i<argc; i++) tcl_argv[i-1]=argv[i];
  else
    tcl_argv[0]=argv[0];

#ifdef MEMDEBUG
  CheckMemusedAtExit checkMemusedAtExit;
#endif

  /* initialise non-GUI part of EcoLab */
    tclcmd() << "if [file exists $ecolab_library/init.tcl]" <<
      "{source $ecolab_library/init.tcl}\n";


#ifdef MPI_SUPPORT
  if (myid>0)
    { 
      do_slave_loop();
      return 0;
    }
  tclcmd() << "handle_error_return\n";
#endif


  if (argc==1) /* call the command line interpreter if no script supplied */
    {
      tclcmd() << "cli\n";
      return 0;
    }

  if (Tcl_EvalFile(interp(),argv[1])!=TCL_OK)
    {
      fprintf(stderr,"%s\n",Tcl_GetStringResult(interp()));
      fprintf(stderr,"%s\n",Tcl_GetVar(interp(),"errorInfo",0)); /* print out trace */
      return 1;  /* not a clean execution */
    }

#ifdef TK
  while (mainWin) /* we are running GUI mode */
    Tcl_DoOneEvent(0); /*Tk_MainLoop();*/
#endif
};

#ifdef TK
NEWCMD(Tkinit,0)
{
  if (mainWin) return;
  if (!Tk_MainWindow(interp()) && Tk_Init(interp())==TCL_ERROR)
    throw error("Error initialising Tk: %s",Tcl_GetStringResult(interp()));
  mainWin = Tk_MainWindow(interp());
  CairoSurface::registerImage();
  /* delete bgerror command for GUI mode */
  //  tclcmd() << "rename bgerror {}\n";
#if BLT
  if (Blt_Init(interp())!=TCL_OK)
    throw error("Error initialising BLT: %s",interp()->result);
#endif  
}

NEWCMD(GUI,0)
{tclcmd() << "source $ecolab_library/Xecolab.tcl\n";}

//this seems to be needed to get Tkinit to function correctly!!
NEWCMD(tcl_findLibrary,-1) {}
#endif

NEWCMD(exit_ecolab,0)
{
#ifdef TK
mainWin=0;
#endif
}

#if MPI_SUPPORT
void ecolab::parsend(int argc, CONST84 char* argv[])
{
  eco_strstream cmd;
  for (int i=0; i<argc; i++) cmd << argv[i];
  MPIbuf() << cmd.str() << bcast(0);
}

void ecolab::parsendf(const char* fmt, ...)
{
  char buffer[MAXPMSG];
  va_list args;
  va_start(args, fmt);
  vsprintf(buffer,fmt,args);
  va_end(args);
  MPIbuf() << string(buffer) << bcast(0);
}

void ecolab::parsend(const string& s) {MPIbuf()<<s<<bcast(0);}
#endif

namespace TCLcmd {

NEWCMD(parallel,-1)
{
  tclcmd c;
  for (int i=1; i<argc; i++) c|'\"'|argv[i]|"\" ";
#ifdef MPI_SUPPORT
  parsend(c.str()); 
#endif
  c<<'\n';
}

NEWCMD(myid,0)
{tclreturn() << ::myid;}

NEWCMD(nprocs,0)
{tclreturn() << ::nprocs;}
}


namespace trap
{
  string sigcmd[32];
  void sighand(int s) {Tcl_Eval(interp(),sigcmd[s].c_str());}
  hash_map<string,int> signum;   /* signal name to number table */
  struct init_t
  {
    init_t()
    {
      signum["HUP"]=	1;	/* Hangup (POSIX).  */
      signum["INT"]=	2;	/* Interrupt (ANSI).  */
      signum["QUIT"]=	3;	/* Quit (POSIX).  */
      signum["ILL"]=	4;	/* Illegal instruction (ANSI).  */
      signum["TRAP"]=	5;	/* Trace trap (POSIX).  */
      signum["ABRT"]=	6;	/* Abort (ANSI).  */
      signum["IOT"]=	6;	/* IOT trap (4.2 BSD).  */
      signum["BUS"]=	7;	/* BUS error (4.2 BSD).  */
      signum["FPE"]=	8;	/* Floating-point exception (ANSI).  */
      signum["KILL"]=	9;	/* Kill, unblockable (POSIX).  */
      signum["USR1"]=	10;	/* User-defined signal 1 (POSIX).  */
      signum["SEGV"]=	11;	/* Segmentation violation (ANSI).  */
      signum["USR2"]=	12;	/* User-defined signal 2 (POSIX).  */
      signum["PIPE"]=	13;	/* Broken pipe (POSIX).  */
      signum["ALRM"]=	14;	/* Alarm clock (POSIX).  */
      signum["TERM"]=	15;	/* Termination (ANSI).  */
      signum["STKFLT"]=	16;	/* Stack fault.  */
      signum["CLD"]=	17;     /* Same as SIGCHLD (System V).  */
      signum["CHLD"]=   17;	/* Child status has changed (POSIX).  */
      signum["CONT"]=	18;	/* Continue (POSIX).  */
      signum["STOP"]=	19;	/* Stop, unblockable (POSIX).  */
      signum["TSTP"]=	20;	/* Keyboard stop (POSIX).  */
      signum["TTIN"]=	21;	/* Background read from tty (POSIX).  */
      signum["TTOU"]=	22;	/* Background write to tty (POSIX).  */
      signum["URG"]=	23;	/* Urgent condition on socket (4.2 BSD).  */
      signum["XCPU"]=  	24;	/* CPU limit exceeded (4.2 BSD).  */
      signum["XFSZ"]=	25;	/* File size limit exceeded (4.2 BSD).  */
      signum["VTALRM"]= 26;	/* Virtual alarm clock (4.2 BSD).  */
      signum["PROF"]=	27;	/* Profiling alarm clock (4.2 BSD).  */
      signum["WINCH"]=  28;	/* Window size change (4.3 BSD, Sun).  */
      signum["POLL"]=	29;/* Pollable event occurred (System V).  */
      signum["IO"]=	29;	/* I/O now possible (4.2 BSD).  */
      signum["PWR"]=	30;	/* Power failure restart (System V).  */
      signum["SYS"]=	31;	/* Bad system call.  */
      signum["UNUSED"]=	31;
    }
  } init;

  NEWCMD(trap,2)     /* trap argv[2] to excute argv[1] */
    {
      int signo = (isdigit(argv[1][0]))? atoi(argv[1]): 
	signum[const_cast<char*>(argv[1])];
      sigcmd[signo]=argv[2];
      signal(signo,sighand);
    }

  void aborthand(int s) {throw error("Fatal Error: Execution recovered");}
  
  NEWCMD(trapabort,-1)
    {
      void (*hand)(int);
      if (argc>1 && strcmp(argv[1],"off")==0) hand=SIG_DFL;
      else hand=aborthand;
      signal(SIGABRT,hand);
      signal(SIGSEGV,hand);
      signal(SIGBUS,hand);
      signal(SIGILL,hand);
    }
}

#if !defined(__MINGW32__) && !defined(__MINGW32_VERSION)
#include <sys/times.h>
#include <time.h>


/* gcc 3 on linux does not include CLK_TCK definition ?? */
#if defined(__linux__) && !defined(CLK_TCK)
extern "C" long int __sysconf (int);
#define CLK_TCK ((__clock_t) __sysconf (2))	/* 2 is _SC_CLK_TCK */
#endif


/* Return total cputime used so far on all threads */
NEWCMD(cputime,0)
{
  PARALLEL;
  struct tms t;
  times(&t);
  int r=t.tms_utime+t.tms_stime;
#ifdef MPI_SUPPORT
  int r1=r;
  MPI_Reduce(&r1,&r,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
#endif
  tclreturn rr;
  rr<<(float)r/CLK_TCK;
}

#include <sys/resource.h>
/* return resident set size consumed */
NEWCMD(rss,0)
{
  PARALLEL;
  rusage u;
  getrusage(RUSAGE_SELF,&u);
  long r=u.ru_maxrss;
#ifdef MPI
  long r1=r;
  MPI_Reduce(&r1,&r,1,MPI_LONG,MPI_SUM,0,MPI_COMM_WORLD);
#endif
  tclreturn() << r;
}

NEWCMD(mem,0)
{
  PARALLEL;
  rusage u;
  getrusage(RUSAGE_SELF,&u);
  long r=u.ru_idrss;
#ifdef MPI
  long r1=r;
  MPI_Reduce(&r1,&r,1,MPI_LONG,MPI_SUM,0,MPI_COMM_WORLD);
#endif
  tclreturn() << r;
}

NEWCMD(stack,0)
{
  PARALLEL;
  rusage u;
  getrusage(RUSAGE_SELF,&u);
  long r=u.ru_isrss;
#ifdef MPI
  long r1=r;
  MPI_Reduce(&r1,&r,1,MPI_LONG,MPI_SUM,0,MPI_COMM_WORLD);
#endif
  tclreturn() << r;
}
#else // not available on Mingw
NEWCMD(cputime,0) {}
NEWCMD(rss,0) {}
NEWCMD(mem,0) {}
NEWCMD(stack,0) {}
#endif



#ifdef READLINE
extern "C" char *readline(char *);
extern "C" void add_history(char *);
#endif

NEWCMD(cli,0)
{
  int braces=0, brackets=0;
  bool inString=false;
  tclcmd cmd;
  cmd << "savebgerror\n";
#ifdef READLINE
  char *c;
  string prompt((const char*)tclvar("argv(0)")); prompt+='>';
  while ( (c=readline(const_cast<char*>(prompt.c_str())))!=NULL && strcmp(c,"exit")!=0)
#else
  char c[512];
  while (fgets(c,512,stdin)!=NULL && strcmp(c,"exit\n")!=0) 
#endif
    {
      // count up number of braces, brackets etc
      for (char* cc=c; *cc!='\0'; ++cc)
        switch(*cc)
          {
          case '{':
            if (!inString) braces++;
            break;
          case '[':
            if (!inString) brackets++;
            break;
          case '}':
            if (!inString) braces--;
            break;
          case ']':
            if (!inString) brackets--;
            break;
          case '\\':
            if (inString) cc++; //skip next character (is literal)
            break;
          case '"':
            inString = !inString;
            break;
          }
      cmd << chomp(c);
      if (!inString && braces<=0 && brackets <=0)
        { // we have a complete command, so attempt to execute it
          try
            {
              cmd.exec();
            }
          catch (std::exception& e) {fputs(e.what(),stderr);}
          catch (...) {fputs("caught unknown exception",stderr);}
          puts(cmd.result.c_str()); 
#if MPI_SUPPORT
          /* process any returned errors */
          try {cmd << "update\n";} catch (std::exception& e) {fputs(e.what(),stderr);}
#endif
        }
#ifdef READLINE
      if (strlen(c)) add_history(c); 
      free(c);
#endif
    }
  cmd << "restorebgerror\n";
}

/*
  string map and string is are not available in TCL 8.0.
  These are substitutes that can be used for the time being
*/

NEWCMD(string_map,2)
{
  int elemc;
  CONST84 char **elem;
  Tcl_SplitList(interp(),const_cast<char*>(argv[1]),&elemc,&elem);
  tclreturn retval;
  for (const char *i=argv[2]; i<argv[2]+strlen(argv[2]);)
    {
      for (int j=0; j<elemc; j+=2)
	if (strncmp(i,elem[j],strlen(elem[j]))==0)
	  {
	    retval | elem[j+1];
	    i+=strlen(elem[j]);
	    goto next_iter;
	  }
      retval | *i;
      i++;
    next_iter:;
    }
  Tcl_Free((char*)elem);
}

#include <ctype.h>
/* -strict and -failindex not implemented. Also class cannot be abbreviated */
NEWCMD(string_is,2)
{
  /* dummy variables for Tcl_Expr calls */
  int dumint;
  long dumlong;
  double dumdouble;
  if (strcmp(argv[1],"alnum")==0) tclreturn() << isalnum(argv[2][0]);
  else if (strcmp(argv[1],"alpha")==0) tclreturn() << isalpha(argv[2][0]);
  else if (strcmp(argv[1],"ascii")==0) tclreturn() << ((argv[2][0] & ~0x7F)!=0);
  else if (strcmp(argv[1],"control")==0) tclreturn() << iscntrl(argv[2][0]);
  else if (strcmp(argv[1],"digit")==0) tclreturn() << isdigit(argv[2][0]);
  else if (strcmp(argv[1],"graph")==0) tclreturn() << isgraph(argv[2][0]);
  else if (strcmp(argv[1],"lower")==0) tclreturn() << islower(argv[2][0]);
  else if (strcmp(argv[1],"print")==0) tclreturn() << isprint(argv[2][0]);
  else if (strcmp(argv[1],"punct")==0) tclreturn() << ispunct(argv[2][0]);
  else if (strcmp(argv[1],"space")==0) tclreturn() << isspace(argv[2][0]);
  else if (strcmp(argv[1],"upper")==0) tclreturn() << isupper(argv[2][0]);
  else if (strcmp(argv[1],"xdigit")==0) tclreturn() << isxdigit(argv[2][0]);
  else if (strcmp(argv[1],"boolean")==0) 
    tclreturn() << (Tcl_ExprBoolean(interp(),const_cast<char*>(argv[2]),&dumint)==TCL_OK);
  else if (strcmp(argv[1],"false")==0) 
    tclreturn() << (Tcl_ExprBoolean(interp(),const_cast<char*>(argv[2]),&dumint)==TCL_OK && !dumint);
  else if (strcmp(argv[1],"true")==0) 
    tclreturn() << (Tcl_ExprBoolean(interp(),const_cast<char*>(argv[2]),&dumint)==TCL_OK && dumint);
  else if (strcmp(argv[1],"double")==0) 
    tclreturn() << (Tcl_ExprDouble(interp(),const_cast<char*>(argv[2]),&dumdouble)==TCL_OK);
  else if (strcmp(argv[1],"integer")==0) 
    tclreturn() << (Tcl_ExprLong(interp(),const_cast<char*>(argv[2]),&dumlong)==TCL_OK);
  else throw error("string_is %s not supported\n",argv[1]);
}

