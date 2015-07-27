/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

// Header file for the parser for GML files
//
// Mark Newman  11 AUG 06

#ifndef _READGML_H
#define _READGML_H

#include <stdio.h>
#include "network.h"

int read_network(NETWORK *network, FILE *stream);
void free_network(NETWORK *network);

#endif
