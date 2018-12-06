/*
  @copyright Russell Standish 2018
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include <Carbon/Carbon.h>
#include <Appkit/NSGraphicsContext.h>
#include <Appkit/NSWindow.h>
#include <tk.h>
#include "getContext.h"
using namespace ecolab;

#include <iostream>
using namespace std;

// This is a private Tk function - so you will need to link to the static libtk.a
extern "C" NSWindow* TkMacOSXDrawableWindow(Drawable drawable);


NSContext::NSContext(Drawable win,int xoffs,int yoffs)
{
  NSWindow* w=TkMacOSXDrawableWindow(win);
  NSGraphicsContext* g=[NSGraphicsContext graphicsContextWithWindow: w];
  graphicsContext=g;
  [g retain];
  context=[g CGContext];
  NSRect contentRect=[w contentRectForFrameRect: w.frame]; // allow for title bar
  CGContextTranslateCTM(context,xoffs,contentRect.size.height-yoffs);
  CGContextScaleCTM(context,1,-1); //CoreGraphics's y dimension is opposite to everybody else's
}

NSContext::~NSContext()
{
  [(NSGraphicsContext*)graphicsContext release];
}
  
