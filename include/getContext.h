/*
  @copyright Russell Standish 2018
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef GETCONTEXT_H
#define GETCONTEXT_H

namespace ecolab
{
  struct NSContext
  {
    CGContextRef context;
    void* graphicsContext;
    NSContext(Drawable win,int xoffs,int yoffs);
    ~NSContext();
  };
}
#endif
