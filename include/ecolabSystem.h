/*
  @copyright Russell Standish 2024
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ECOLAB_SYSTEM_H
#define ECOLAB_SYSTEM_H
#include <string>

namespace ecolab
{
  struct System
  {
    static std::string ecolabLibFilename();
    static std::string ecolabLib();
  };
}

#include "ecolabSystem.cd"
#endif
