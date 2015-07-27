/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "cairo_base.h"
#include "tcl++.h"
#include "ecolab_epilogue.h"
using namespace ecolab::cairo;
using namespace ecolab;

struct TestCairo: public CairoImage
{
  virtual void draw() 
  {
    cairo_t* cairo=cairoSurface->cairo();
    cairo_new_path(cairo);
    cairo_arc(cairo, 0,0,250,0,2*M_PI);
    cairo_rectangle(cairo,-249,-249,500,500);
    cairo_stroke(cairo);
    cairo_select_font_face(cairo, "serif", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cairo, 100);
    cairo_move_to(cairo, 0, 0);
    cairo_show_text(cairo, "hello world");
    cairoSurface->blit();
  }
};

int registerTestCairo()
{
  static Tk_ItemType testCairoType = cairoItemType();
  testCairoType.name=const_cast<char*>("testCairo");
  testCairoType.createProc=createImage<TestCairo>;
  Tk_CreateItemType(&testCairoType);
  return 0;
}

// note: registerTestCairo needs to be called after main() has started
//static int regTestCairo=registerTestCairo();
NEWCMD(createTestCairo,0)
{registerTestCairo();}


// pass in an image name here
NEWCMD(testCairo,1)
{
  TestCairo testC;
  testC.draw();
  TkPhotoSurface surf(Tk_FindPhoto(interp(), argv[1]), 0);
  assert(surf.width()==surf.height());
  double w=surf.width();

  double d = testC.distanceFrom(0.9*w,0.9*w);
  double expected = (800*M_SQRT2-500)/2000.0*w;
  //  assert(fabs(d-expected) < 0.01);

  

  assert(testC.inClip(.9*w,.9*w,w,w)==-1);
  assert(testC.inClip(.5*w,.5*w,.9*w,.9*w)==0);
  assert(testC.inClip(.5*w,.5*w,.6*w,.6*w)==1);
}
