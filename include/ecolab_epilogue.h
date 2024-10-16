/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/**\file
\brief Epilogue header for EcoLab projects

Put this after including all .cd files
*/
#ifdef GRAPHCODE_H
#include "graphcode.cd"
#endif
#include "classdesc_epilogue.h"

#ifdef ECOLAB_H
#include "pack_base.h"

namespace ecolab
{
  template <class M>
  void Model<M>::checkpoint(const char* fileName)
  {
    pack_t b(fileName,"w");
    b<<static_cast<M&>(*this);
  }

  template <class M>
  void Model<M>::restart(const char* fileName)
  {
    pack_t b(fileName,"r");
    b>>static_cast<M&>(*this);
  }
}
#endif
