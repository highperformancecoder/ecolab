/*
  @copyright Russell Standish 2024
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <Python.h>
#include <boost/locale.hpp>
#ifdef MPI_SUPPORT
#include "classdescMP.h"
#endif
#include "ecolab_epilogue.h"

#include <string>
#include <vector>
using namespace std;
using boost::locale::conv::utf_to_utf;

int main(int argc, char* argv[])
{
#ifdef MPI_SUPPORT
  classdesc::MPISPMD spmd(argc, argv);
#endif
  // convert arguments to UTF-16 equivalents
  // Python 3.8 and later have Py_BytesMain, which obviates this code
  vector<wstring> wargs;
  for (int i=0; i<argc; ++i)
    wargs.push_back(utf_to_utf<wchar_t>(argv[i]));
  wchar_t* wargsData[argc];
  for (int i=0; i<argc; ++i)
    wargsData[i]=const_cast<wchar_t*>(wargs[i].c_str());
  return Py_Main(argc,wargsData);
}
