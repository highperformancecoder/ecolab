/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "graph.h"
#include "eco_strstream.h"
#include "netcomplexity.h"
#include "testgraph.h"
#include "testgraph.cd"
#include "ecolab_epilogue.h"

using namespace ecolab;

Testgraph tg;
CLASSDESC_ADD_GLOBAL(tg);
using DiGraph1=DiGraph;
CLASSDESC_DECLARE_TYPE(DiGraph1);
CLASSDESC_PYTHON_MODULE(testgraph);


