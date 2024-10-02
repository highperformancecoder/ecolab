/*
  @copyright Russell Standish 2024
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <Python.h>
#include <boost/locale.hpp>
#include "ecolab.h"

#ifdef MPI_SUPPORT
#include "classdescMP.h"

void throw_MPI_errors(MPI_Comm * c, int *code, ...)
{
  char errstr[MPI_MAX_ERROR_STRING+1];
  int length;
  MPI_Error_string(*code,errstr,&length);
  errstr[length]='\0';
  puts(errstr);
  PyErr_SetString(PyExc_RuntimeError, errstr);
}



#endif
#include "ecolab_epilogue.h"

#include <iostream>
#include <string>
#include <vector>
using namespace std;
using namespace ecolab;
using boost::locale::conv::utf_to_utf;

struct Python
{
  Python() {Py_Initialize();}
  ~Python() {Py_FinalizeEx();}
};

int main(int argc, char* argv[])
{
#ifdef MPI_SUPPORT
  classdesc::MPISPMD spmd(argc, argv);
  MPI_Errhandler errhandler;
#if MPI_VERSION>=2
  MPI_Comm_create_errhandler((MPI_Comm_errhandler_function*)throw_MPI_errors,&errhandler);
  MPI_Comm_set_errhandler(MPI_COMM_WORLD,errhandler);
#else
  MPI_Errhandler_create((MPI_Handler_function*)throw_MPI_errors,&errhandler);
  MPI_Errhandler_set(MPI_COMM_WORLD,errhandler);
#endif
#endif
  if (argc<2) return 0; // nothing to do
  // this assumes that all processes see the same filesystem, and
  // potentially the same CWD if the argument is a relative filename
  FILE* script=fopen(argv[1],"rb");
  if (!script) {
    cerr<<"Failed to open: "<<argv[1]<<endl;
    return 1; // invalid file
  }
  Python python;
  addEcoLabPath();
  PyImport_ImportModule("ecolab");
  registerParallel();
  if (auto path=PySys_GetObject("path"))
    {
      PyList_Append(path,PyUnicode_FromString("."));
      PyList_Append(path,PyUnicode_FromString(ECOLAB_HOME"/lib"));
    }
  auto pyArgv=PyList_New(0);
  for (int i=1; i<argc; ++i)
    PyList_Append(pyArgv, PyUnicode_FromString(argv[i]));
  PySys_SetObject("argv",pyArgv);

  int err=0;
  if (err=PyRun_SimpleFile(script,argv[1]))
    PyErr_Print();
#ifdef MPI_SUPPORT
  // terminate any parallel region
  if (myid()==0) MPIbuf()<<string("return")<<bcast(0);
#endif
  return err;
}
