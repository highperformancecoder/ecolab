/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/* tcl++.h

Contains 4 concepts - a NEWCMD macro for declaring Tcl procedures, a
tclvar class for accessing TCL variables as though they were C
variables, a tclcmd class that turns the TCL interpreter stream
into a simple I/O stream and tclindex, a simple iterator through a TCL array */

/**\file
\brief Basic C++ TCL access
*/

#ifndef TCLPP_H
#define TCLPP_H

#if MPI_SUPPORT
#undef HAVE_MPI_CPP
//#undef SEEK_SET
//#undef SEEK_CUR
//#undef SEEK_END
#include <mpi.h>
#endif

#include "classdesc_access.h"
#include "pack_base.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>
#include <tcl.h>

#include <string>

/* for Tcl 8.4 compatibility */
#ifndef CONST84
#define CONST84
#endif

#ifdef TK
#include <tk.h>
/* undefine all those spurious macros X11 defines - we don't need 'em! */
#undef Bool
#undef Status
#undef True
#undef False
#undef QueuedAlready
#undef QueuedAfterReading
#undef QueuedAfterFlush
#undef ConnectionNumber
#undef RootWindow
#undef DefaultScreen
#undef DefaultRootWindow
#undef DefaultVisual
#undef DefaultGC
#undef BlackPixel
#undef WhitePixel
#undef AllPlanes 
#undef QLength
#undef DisplayWidth
#undef DisplayHeight
#undef DisplayWidthMM
#undef DisplayHeightMM
#undef DisplayPlanes
#undef DisplayCells
#undef ScreenCount
#undef ServerVendor
#undef ProtocolVersion
#undef ProtocolRevision
#undef VendorRelease
#undef DisplayString
#undef DefaultDepth
#undef DefaultColormap
#undef BitmapUnit
#undef BitmapBitOrder
#undef BitmapPad
#undef ImageByteOrder
#undef NextRequest
#undef LastKnownRequestProcessed
#undef ScreenOfDisplay
#undef DefaultScreenOfDisplay
#undef DisplayOfScreen
#undef RootWindowOfScreen
#undef BlackPixelOfScreen
#undef WhitePixelOfScreen
#undef DefaultColormapOfScreen
#undef DefaultDepthOfScreen
#undef DefaultGCOfScreen
#undef DefaultVisualOfScreen
#undef WidthOfScreen
#undef HeightOfScreen
#undef WidthMMOfScreen
#undef HeightMMOfScreen
#undef PlanesOfScreen
#undef CellsOfScreen
#undef MinCmapsOfScreen
#undef MaxCmapsOfScreen
#undef DoesSaveUnders
#undef DoesBackingStore
#undef EventMaskOfScreen
#undef XAllocID
#undef XNRequiredCharSet
#undef XNQueryOrientation 
#undef XNBaseFontName
#undef XNOMAutomatic
#undef XNMissingCharSet
#undef XNDefaultString
#undef XNOrientation 
#undef XNDirectionalDependentDrawing
#undef XNContextualDrawing
#undef XNFontInfo 
#undef XIMPreeditArea		
#undef XIMPreeditCallbacks	
#undef XIMPreeditPosition	
#undef XIMPreeditNothing	
#undef XIMPreeditNone		
#undef XIMStatusArea		
#undef XIMStatusCallbacks	
#undef XIMStatusNothing	
#undef XIMStatusNone		
#undef XNVaNestedList 
#undef XNQueryInputStyle 
#undef XNClientWindow 
#undef XNInputStyle 
#undef XNFocusWindow 
#undef XNResourceName 
#undef XNResourceClass 
#undef XNGeometryCallback 
#undef XNDestroyCallback 
#undef XNFilterEvents
#undef XNPreeditStartCallback 
#undef XNPreeditDoneCallback 
#undef XNPreeditDrawCallback 
#undef XNPreeditCaretCallback 
#undef XNPreeditStateNotifyCallback 
#undef XNPreeditAttributes
#undef XNStatusStartCallback 
#undef XNStatusDoneCallback
#undef XNStatusDrawCallback 
#undef XNStatusAttributes 
#undef XNArea 
#undef XNAreaNeeded
#undef XNSpotLocation
#undef XNColormap 
#undef XNStdColormap 
#undef XNForeground 
#undef XNBackground 
#undef XNBackgroundPixmap 
#undef XNFontSet 
#undef XNLineSpace 
#undef XNCursor 
#undef XNQueryIMValuesList 
#undef XNQueryICValuesList 
#undef XNVisiblePosition 
#undef XNR6PreeditCallback 
#undef XNStringConversionCallback 
#undef XNStringConversion 
#undef XNResetState 
#undef XNHotKey 
#undef XNHotKeyState 
#undef XNPreeditState 
#undef XNSeparatorofNestedList 
#undef XBufferOverflow		
#undef XLookupNone		
#undef XLookupChars		
#undef XLookupKeySym		
#undef XLookupBoth		
#undef XIMReverse		
#undef XIMUnderline		 
#undef XIMHighlight		
#undef XIMPrimary	 	
#undef XIMSecondary		
#undef XIMTertiary	 	
#undef XIMVisibleToForward 	
#undef XIMVisibleToBackword 	
#undef XIMVisibleToCenter 	
#undef	XIMPreeditUnKnown	
#undef	XIMPreeditEnable	
#undef	XIMPreeditDisable	
#undef	XIMInitialState		
#undef	XIMPreserveState	
#undef	XIMStringConversionLeftEdge	
#undef	XIMStringConversionRightEdge	
#undef	XIMStringConversionTopEdge	
#undef	XIMStringConversionBottomEdge	
#undef	XIMStringConversionConcealed	
#undef	XIMStringConversionWrapped	
#undef	XIMStringConversionBuffer	
#undef	XIMStringConversionLine		
#undef	XIMStringConversionWord		
#undef	XIMStringConversionChar		
#undef	XIMStringConversionSubstitution	
#undef	XIMStringConversionRetrieval	
#undef	XIMStringConversionRetrival	
#undef	XIMHotKeyStateON	
#undef	XIMHotKeyStateOFF	

#undef Status // conflict between X11 usage and MPI++ usage
#endif
#include <time.h>
#include "Realloc.h"
#include "eco_strstream.h"
#include "error.h"

namespace ecolab
{
  using std::string;

  extern bool interpExiting;
  void interpExitProc(ClientData cd);

  /// default interpreter. Set to NULL when interp() is destroyed
  inline Tcl_Interp *interp()
  {
    // TODO - this idea doesn't seem to work when exit is called in a script
    //    static shared_ptr<Tcl_Interp> interp(Tcl_CreateInterp(), Tcl_DeleteInterp);
    static Tcl_Interp* interp=Tcl_CreateInterp();
    Tcl_CreateExitHandler(interpExitProc, 0);
    return interp;
  }
  /// main window of application
  //  extern Tk_Window mainWin;    


  /* these are defined to default values, even if MPI is false */
  /// MPI process ID and number of processes
  extern unsigned myid, nprocs;

  /// semaphore controlling background event process during command
  /// execution. When zero, background events are processed
  extern int processEventsNest;
  struct DisableEventProcessing
  {
    DisableEventProcessing() {processEventsNest++;}
    ~DisableEventProcessing() {processEventsNest--;}
  };

#if MPI_SUPPORT
  /// Run a TCL command on all processors
#define PARALLEL if (myid==0) parsend(argc,argv);

  /// maximum size of TCL command passed to slave processors 
#define MAXPMSG 1024
#define TAG_PUSH 1 
  void parsend(int,CONST84 char**);
  void parsendf(const char*,...); /// printf style multiargument
  void parsend(const std::string&);
#else
#define PARALLEL
#endif

  /// test whether command is not already defined 
  inline int TCL_newcommand(const char *c)
  { 
    Tcl_CmdInfo dum;
    return !Tcl_GetCommandInfo(interp(),const_cast<char*>(c),&dum);
  }


  int TCL_proc(ClientData cd, Tcl_Interp *interp, int argc, CONST84 char **argv);

  /* base class for tcl++ and TCL_obj commands */
  class cmd_data 
  {
  public:
    typedef enum {invalid, memvoid, mem, func, nonconst} functor_class;
    int nargs;
    string name;
    /// true if this command doesn't affect the object's (or global state)
    bool is_const; 
    virtual void proc(int argc, CONST84 char** argv)=0;
    // TODO possibly delegate to above?
    virtual void proc(int argc, Tcl_Obj *const argv[]) {
      throw error("proc not implemented");};
    cmd_data(const char* nm, int na=-1):  nargs(na), name(nm), is_const(false)
                                         /* by default, don't check arg count */
    {
      assert(TCL_newcommand(nm));
    }
    cmd_data(): nargs(-1) {}
    virtual ~cmd_data() {}
  };


  class tclpp_cd: public cmd_data
  {
    void (*procptr)(int,CONST84 char**);
  public:
    void proc(int argc, CONST84 char** argv) {procptr(argc,argv);}
    void proc(int argc, Tcl_Obj *const argv[]) {}
    tclpp_cd(const char* nm, int na, void (*p)(int,CONST84 char**)): 
      cmd_data(nm,na), procptr(p)
    {Tcl_CreateCommand(interp(),const_cast<char*>(nm),(Tcl_CmdProc*)TCL_proc,(ClientData)this,NULL);}
  };

  /// set the value of the TCL variable ecolab_library
  int setEcoLabLib(const char* path);

  /// define ecolab_library in user models
#ifdef ECOLAB_LIB
#define DEFINE_ECOLAB_LIBRARY                                     \
  namespace {                                                     \
    int dum=ecolab::setEcoLabLib(ECOLAB_LIB);                     \
  }
#else
#define DEFINE_ECOLAB_LIBRARY
#endif


  /** Macro for declaring TCL commands: takes a name and the number of
      arguments expected. The command will return and error if the number
      of arguments is not what is expected. A declaration of -1 for the
      expected number of arguments disables this checking */

#define NEWCMD(name,nargs)                              \
  static void name(int argc, CONST84 char* argv[]);     \
  namespace name##_ns {                                 \
    DEFINE_ECOLAB_LIBRARY;                              \
    ecolab::tclpp_cd name##data(#name,nargs,name);      \
  }                                                     \
  static void name(int argc,CONST84 char* argv[])              
  
  class tclindex;

  /// TCL variable class 

  /** A class implementing the concept of a TCL variable. Here, if the
      programmer declares 

      tclvar hello="hello"; 

      then the variable hello can be used just like a normal C variable in
      expression such as 

      floatvar=hello*3.4.  

      However the difference is that hello is bound to a TCL variable called
      hello, which can be accessed from TCL scripts.

  */

  class tclvar
  { 

    CLASSDESC_ACCESS(tclvar);
    string name;  
    inline double dget(void);  
    inline double dput(double x);

  public:

    /* constructors */
    tclvar() {}
    /// TCL var name this i sbound to. If val not null, then initialise the var to \a val
    tclvar(const string& nm, const char* val=NULL);

    ///tclvars may be freely mixed with arithmetic  expressions 
    double operator=(double x) {return dput(x);}
    const char* operator=(const char* x) 
    {return interpExiting? "": Tcl_SetVar(interp(),name.c_str(),x,TCL_GLOBAL_ONLY);}

    const char* operator+(const char* x) 
    {
      string t(operator const char*());
      t+=x; *this=t.c_str();
      return t.c_str();
    } 
    ///
    operator double () {return dget();}
    ///
    operator const char* () 
    {return interpExiting? "": Tcl_GetVar(interp(),name.c_str(),TCL_GLOBAL_ONLY);}
    ///
    operator int() {return (int)dget();}
    operator unsigned () {return (unsigned)dget();}

    ///
    double operator++()   {return dput(dget()+1);}
    ///
    double operator++(int){double tmp; tmp=dget(); dput(tmp+1); return tmp;}
    ///
    double operator--()   {return dput(dget()-1);}
    ///
    double operator--(int){double tmp; tmp=dget(); dput(tmp-1); return tmp;} 
    ///
    double operator+=(double x) {return dput(dget()+x);}
    ///
    double operator-=(double x) {return dput(dget()-x);}
    ///
    double operator*=(double x) {return dput(dget()*x);}
    ///
    double operator/=(double x) {return dput(dget()/x);}

    /// arrays can be indexed either by integers, or by strings 
    tclvar operator[](int index);
    /// arrays can be indexed either by integers, or by strings 
    tclvar operator[](const string& index);

    ///size  of arrays 
    int size();  

    friend bool exists(const tclvar& x);
    friend class tclindex;
  };

  /* space for a char[] variable to hold a double value in text form */
#define BUFSIZE 20 

  inline
  double tclvar::dget(void)  
  {
    if (interpExiting) return 0;
    double val;
    int ival;
    char* tclval;
    tclval=const_cast<char*>(Tcl_GetVar(interp(),name.c_str(),TCL_GLOBAL_ONLY));
    if (tclval!=NULL)
      {
        if (Tcl_GetDouble(interp(),tclval,&val)!=TCL_OK)
          {
            if (Tcl_GetBoolean(interp(),tclval,&ival)!=TCL_OK)
              throw error("%s as the value of %s\n",Tcl_GetStringResult(interp()),name.c_str());
            else
              return ival;
          }
      }
    else
      throw error("TCL Variable %s is undefined\n",name.c_str());
    return val;
  }

  inline
  double tclvar::dput(double x)
  { 
    if (interpExiting) return x;
    eco_strstream value;
    value << x;
    std::string v(value.str());
    Tcl_SetVar(interp(),name.c_str(),v.c_str(),TCL_GLOBAL_ONLY);
    return x;
  }

  ///Check if a TCL variable exists
  inline
  bool exists(const tclvar& x)
  {return interpExiting? false: Tcl_GetVar(interp(),x.name.c_str(),TCL_GLOBAL_ONLY)!=NULL;}

  inline
  tclvar::tclvar(const string& nm, const char* val): name(nm) 
  { 
    /* TCL 8.3 or earlier does not declare this stuff const */
    if (val!=NULL) Tcl_SetVar(interp(),const_cast<char*>(name.c_str()),
                              const_cast<char*>(val),0);
  }

  inline
  tclvar tclvar::operator[](int index)
  {
    std::ostringstream os;
    os<<index;
    return operator[](os.str());
  }

  inline
  tclvar tclvar::operator[](const string& index)
  {
    tclvar tmp(name);
    if (tmp.name.find('(')!=string::npos)  /* already an indexed element */
      tmp.name=tmp.name.substr(0, tmp.name.find(')'))+index+")";
    else
      tmp.name+="("+index+")";
    return tmp;
  }

  // enable printing of tclvars through the stream process 
  inline
  std::ostream& operator<<(std::ostream& stream, tclvar x)
  { return stream << (const char*) x;}

#define DBLSIZ 16 /* enough characters to hold a string rep of a double */
#define INTSIZ 12 /* enough characters to hold a string rep of an int */

  /* remove trailing newline, if any. */
  inline std::string chomp(const std::string& s)
  {
    if (s[s.length()-1]=='\n') return s.substr(0,s.length()-1);
    else return s;
  }
      

  class tclcmd: public eco_strstream
  {
    CLASSDESC_ACCESS(tclcmd);
  public:
    string result;   /* The result of the previous command is placed here */
    void exec()
    {
      if (interpExiting) return;
      std::string s=chomp(str());
      if (Tcl_Eval(interp(),s.c_str())!=TCL_OK) 
        {
          error e("%s->%s\n",s.c_str(),Tcl_GetStringResult(interp()));
          clear();
          throw e;
        }
      result=Tcl_GetStringResult(interp());
      clear();   /* null out string ready for next command */
    }

    template<class T>
    tclcmd& operator<<(T x) 
    {
      if (str()[0]=='\0')
        return (*this)|x;
      else
        return (*this)|" "|x;
    }


    template <class T>
    tclcmd& operator|(T x) {eco_strstream::operator|(x); return *this;}

    tclcmd& operator|(char x) 
    {
      if (x=='\n') 
        exec(); 
      else 
        return (tclcmd&) eco_strstream::operator|(x);
      return *this;
    }

    tclcmd& operator|(const char* cmd) 
    {
      eco_strstream::operator|(cmd);
      if (cmd[strlen(cmd)-1]=='\n') exec();
      return *this;
    }
    tclcmd& operator|(char* cmd) {return operator|((const char*) cmd);}
 
  };

  /// An RAII class for returning values to TCL
  class tclreturn: public eco_strstream 
  {
  public:
    ~tclreturn() {
      if (interpExiting) return;
      std::string tmps=str();
      Tcl_SetResult(interp(),const_cast<char*>(tmps.c_str()),TCL_VOLATILE);}
  };

  /** index through TCL arrays: example usage - obtaining product of elements in 
      array 
      \begin{verbatim}
      for (ncells *= (int)idx.start(dims); !idx.last(); ncells *= (int)idx.incr() );
      \end{verbatim}
  */
  class tclindex  
  {
    CLASSDESC_ACCESS(tclindex);
    std::string searchid;
    std::string arrayname;
  public:
    ~tclindex() {done();}
    //start the indexing
    tclvar start(const tclvar&);
    //finish up
    inline void done();
    //get next element in array
    inline tclvar incr();
    tclvar incr(const tclvar& x) {return incr();}  /* ignores argument */
    //return true if this is the last element in array
    int last();
  };

  inline
  void tclindex::done()
  {
    tclcmd cmd;
    if (!searchid.empty())
      {
        cmd << "array donesearch " << arrayname << searchid << "\n";
        arrayname.clear();
        searchid.clear();
      }
  }

  inline 
  tclvar tclindex::start(const tclvar& x)
  {
    tclcmd cmd;
    tclvar r;
    /* check if x is actually an array variable */
    cmd << "array exists " << x.name << "\n";
    if (!atoi(cmd.result.c_str())) return x;

    done();  /* ensure any previous invocation is cleaned up */
    cmd << "array startsearch " << x.name << "\n";
    arrayname = x.name;
    searchid = cmd.result;
    r=incr();    /* work around a compiler bug in gcc 2.8.1??? */
    return r;
  }
  
  inline
  tclvar tclindex::incr()
  {
    tclcmd cmd;
    tclvar r;
  
    if (searchid.empty()) throw error("tclindex not initialized");
    cmd << "array nextelement " << arrayname << searchid << "\n";
    r.name=arrayname+"("+cmd.result+")";
    return r;
  }

  inline 
  int tclvar::size()
  {
  
    tclcmd cmd;
    cmd << "array size " << name << "\n";
    return atoi(cmd.result.c_str());
  }


  inline 
  int tclindex::last()
  {
    tclcmd cmd;
    if (!searchid.empty())
      {
        cmd << "array anymore " << arrayname << searchid << "\n";
        return !atoi(cmd.result.c_str());
      }
    else
      return 1;
  }
}
#endif
