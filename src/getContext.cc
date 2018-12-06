/*
  @copyright Russell Standish 2018
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <Carbon/Carbon.h>
#include <Appkit/NSGraphicsContext.h>
#include <tk.h>
#include "getContext.h"
using namespace ecolab;

extern "C" NSWindow* TkMacOSXDrawableWindow(Drawable drawable);


NSContext::NSContext(Drawable win,int xoffs,int yoffs,int height)
{
  NSWindow* w=TkMacOSXDrawableWindow(win);
  NSGraphicsContext* g=[NSGraphicsContext graphicsContextWithWindow: w];
  graphicsContext=g;
  [g retain];
  context=[g CGContext];
  CGContextScaleCTM(context,1,-1);
  CGContextTranslateCTM(context,0,-height);
}

NSContext::~NSContext()
{
  [(NSGraphicsContext*)graphicsContext release];
}
  
