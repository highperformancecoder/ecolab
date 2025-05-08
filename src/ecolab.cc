/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "tcl++.h"
#define TK 1
#undef None
#include "cairoSurfaceImage.h"
#undef None
#include "plot.h"
#include "pythonBuffer.h"
#include "ecolab.h"
#ifdef MPI_SUPPORT
#include "graphcode.h"
#endif
#include "classdesc.h"
#include "ecolab_epilogue.h"
using namespace ecolab;
using namespace std;

#include <dlfcn.h>

#ifdef SYCL_LANGUAGE_VERSION
using namespace sycl;
bool ecolab::syclQDestroyed=false;
namespace
{
  struct SyclQ: public queue
  {
    SyclQ(): queue(default_selector_v) {}
    ~SyclQ() {syclQDestroyed=true;}
  };
}

queue& ecolab::syclQ() {
  static SyclQ q;
  return q;
}

void* ecolab::reallocSycl(void* pp,size_t s)
{
  size_t* p=reinterpret_cast<size_t*>(pp);
  size_t prevSz=0;
  if (p)
    prevSz=*--p;

  if (!s && p)
    {
      free(p,syclQ());
      return nullptr;
    }

  if (prevSz>=s) return pp; // nothing further to do
  auto r=reinterpret_cast<size_t*>(malloc_shared(s+sizeof(size_t),syclQ()));
  if (r)
    {
      // stash allocation size at beginning of allocated region
      *r++=s;
      if (p)
        {
          memcpy(r,pp,prevSz);
          free(p,syclQ());
        }
    }
  return r;
}
#endif  

namespace ecolab
{
  bool interpExiting=false;
  void interpExitProc(ClientData cd) {}
  
#ifdef MPI_SUPPORT
  unsigned myid() {return graphcode::myid();}
  unsigned nprocs() {return graphcode::nprocs();}
#else
  unsigned myid() {return 0;}
  unsigned nprocs() {return 1;}
#endif

  
  const char* TCL_args::str() 
#if (TCL_MAJOR_VERSION==8 && TCL_MINOR_VERSION==0)
    {return Tcl_GetStringFromObj(pop_arg(),NULL);}
#else
    {return Tcl_GetString(pop_arg());}
#endif

  int addEcoLabPath()
  {
    if (Py_IsInitialized())
      if (auto path=PySys_GetObject("path"))
        PyList_Append(path,PyUnicode_FromString(ECOLAB_HOME"/lib"));
    return 0;
  }

  namespace
  {
    int setPath=addEcoLabPath();

    /// Python object to implement MPI process control from Python
    struct Parallel: public CppPyObject
    {
      PyObject* target=nullptr;
    };

    static PyObject* exit(Parallel* self, PyObject*)
    {
#ifdef MPI_SUPPORT
      if (myid()==0)
        MPIbuf()<<string("return")<<bcast(0);
#endif
      self->target=nullptr;
      return Py_None; 
    }

    PyMethodDef parallelMethods[]={
      {"exit",(PyCFunction)exit,METH_NOARGS,"Signal workers to knock off"},
      {nullptr, nullptr, 0, nullptr}
    };

    struct ParallelType: public PyTypeObject
    {
      static PyObject* call(PyObject* self, PyObject* args, PyObject *kwargs)
      {
        if (myid()>0)
          {
            PyErr_SetString(PyExc_RuntimeError, "Must be called on master process");
            return nullptr;
          }
        if (!self)
          {
            PyErr_SetString(PyExc_RuntimeError, "No target object supplied");
            return nullptr;
          }
        if (!PySequence_Check(args) || PySequence_Size(args)<1)
          {
            PyErr_SetString(PyExc_RuntimeError, "Incorrect arguments");
            return nullptr;
          }
        auto target=static_cast<Parallel*>(self)->target;
        if (!target)
          {
            PyErr_SetString(PyExc_RuntimeError, "Workers exited");
            return nullptr;
          }
#ifdef MPI_SUPPORT
        std::string method=PyUnicode_AsUTF8(PySequence_GetItem(args,0));
        // we need to pickle the arguments and kwargs, so leave argument support until later
        MPIbuf b; b<<method<<bcast(0);
#endif
        return PyObject_Call(PyObject_GetAttr(target,PySequence_GetItem(args,0)),PyTuple_New(0),nullptr);
      }

      static int init(PyObject* self, PyObject* args, PyObject*)
      {
        if (!PySequence_Check(args) || PySequence_Size(args)<1)
          {
            PyErr_SetString(PyExc_RuntimeError, "Incorrect arguments");
            return -1;
          }
        auto target=static_cast<Parallel*>(self)->target=PySequence_GetItem(args,0);
#ifdef MPI_SUPPORT
        for (;myid()>0;)
          {
            MPIbuf b; b.bcast(0);
            std::string method; b>>method;
            if (method=="return") break;
            // we need to pickle the arguments and kwargs, so leave argument support until later
            PyObject_Call(PyObject_GetAttrString(target,method.c_str()),PyTuple_New(0),nullptr);
          }
#endif
        return 0;
      }

      static void finalize(PyObject* self) {exit(static_cast<Parallel*>(self),nullptr);}

      ParallelType()
      {
        memset(this,0,sizeof(PyTypeObject));
        Py_INCREF(this);
        tp_name="Parallel";
        tp_methods=parallelMethods;
        tp_call=call;
        tp_init=init;
        tp_alloc=PyType_GenericAlloc;
        tp_new=PyType_GenericNew;
        tp_finalize=finalize;
        tp_basicsize=sizeof(Parallel);
        PyType_Ready(this);
      }

    };
  }

  void registerParallel()
  {
    static ParallelType parallelType;
    PyModule_AddObject(pythonModule,"Parallel",reinterpret_cast<PyObject*>(&parallelType));
  }
  CLASSDESC_ADD_FUNCTION(registerParallel);

 
  std::string ecolabHome=ECOLAB_HOME;
  CLASSDESC_ADD_GLOBAL(ecolabHome);
  CLASSDESC_ADD_GLOBAL(array_urand);
  CLASSDESC_ADD_GLOBAL(array_grand);
  CLASSDESC_ADD_FUNCTION(myid);
  CLASSDESC_ADD_FUNCTION(nprocs);
  namespace python
  {
    using Plot=PlotSurface;
    CLASSDESC_DECLARE_TYPE(Plot);
  }

  // device SYCL kernels are running on
  std::string device() {
#ifdef SYCL_LANGUAGE_VERSION
    return syclQ().get_device().get_info<info::device::name>();
#else
    return "CPU";
#endif
  }
  CLASSDESC_ADD_FUNCTION(device);
}

CLASSDESC_PYTHON_MODULE(ecolab);


// place for initialising any EcoLab extensions to the TCL
// interpreter, to be called by tkinter's Tk() object
extern "C" int Ecolab_Init(Tcl_Interp* interp)
{
  CairoSurface::registerImage();
  return TCL_OK;
}

// some linkers add an _
extern "C" int _Ecolab_Init(Tcl_Interp* interp) {return Ecolab_Init(interp);}

