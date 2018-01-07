#include "pango.h"
#include "cairo_base.h"
#include <ecolab_epilogue.h>

#include <iostream>
#include <assert.h>
using ecolab::Pango;
using ecolab::cairo::Surface;
using namespace std;

int main()
{
  Surface surface(cairo_recording_surface_create(CAIRO_CONTENT_COLOR,NULL));
  Pango pango(surface.cairo());
  pango.setMarkup("a test string");
  assert(pango.idxToPos(2)<pango.idxToPos(4));
//  for (size_t i=0; i<13; ++i)
//    cout << i << " " << pango.idxToPos(i) << " " << pango.posToIdx(pango.idxToPos(i)) << endl;
  assert(pango.posToIdx(pango.idxToPos(3))==3);
}
