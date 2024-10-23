/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

// include wrapper that includes necessaries for the pack descriptor
// pack_stream is included in classdesc_epilogue if this file has been included
#ifndef PACK_H
#define PACK_H
#include "pack_base.h"
//#include "ref.h"
#include "pack_stl.h"
#ifndef SYCL_LANGUAGE_VERSION
// SYCL cannot call recursive functions, so serialisation of graph structures is impossible
#include "pack_graph.h"
#endif
#endif
