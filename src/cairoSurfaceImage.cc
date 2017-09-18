/*
  @copyright Russell Standish 2017
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#if defined(CAIRO) && defined(TK)
#include "cairoSurfaceImage.h"
#include "tcl++.h"

#ifdef _WIN32
#undef Realloc
#include <windows.h>
#endif

#include "ecolab_epilogue.h"
#define USE_WIN32_SURFACE defined(CAIRO_HAS_WIN32_SURFACE) && !defined(__CYGWIN__)

#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo/cairo-xlib.h>
#endif
#if USE_WIN32_SURFACE
#include <cairo/cairo-win32.h>
// undocumented internal function for extracting the HDC from a Drawable
extern "C" HDC TkWinGetDrawableDC(Display*, Drawable, void*);
extern "C" HDC TkWinReleaseDrawableDC(Drawable, HDC, void*);
#endif

#if defined(MAC_OSX_TK)
#include <Carbon/Carbon.h>
#include <cairo/cairo-quartz.h>
#include "getContext.h"
#endif

using namespace ecolab;

namespace
{
    struct TkWinSurface: public ecolab::cairo::Surface
    {
      CairoSurface& csurf;
      Tk_ImageMaster imageMaster;
      TkWinSurface(CairoSurface& csurf, Tk_ImageMaster imageMaster, cairo_surface_t* surf):
        cairo::Surface(surf), csurf(csurf),  imageMaster(imageMaster) {}
      void requestRedraw() {
        Tk_ImageChanged(imageMaster,-1000000,-1000000,2000000,2000000,2000000,2000000);
      }
      void blit() {cairo_surface_flush(surface());}
    };

    struct CD
    {
      Tk_Window tkWin;
      Tk_ImageMaster master;
      CairoSurface& csurf;
      CD(Tk_Window tkWin, Tk_ImageMaster master, CairoSurface& csurf):
        tkWin(tkWin), master(master), csurf(csurf) {}
    };
    
    // Define a new image type that renders a minsky::Canvas
    int createCI(Tcl_Interp* interp, const char* name, int objc, Tcl_Obj *const objv[],
                 const Tk_ImageType* typePtr, Tk_ImageMaster master, ClientData *masterData)
    {
      try
        {
          TCL_args args(objc,objv);
          string canvas=args; // arguments should be something like -surface minsky.canvas
          if (TCL_obj_hash::mapped_type mb=TCL_obj_properties()[canvas])
            if (CairoSurface* csurf=mb->memberPtrCasted<CairoSurface>())
              {
                *masterData=new CD(0,master,*csurf);
                return TCL_OK;
              }
          Tcl_AppendResult(interp,"Not a CairoSurface",NULL);
          return TCL_ERROR;
        }
      catch (const std::exception& e)
        {
          Tcl_AppendResult(interp,e.what(),NULL);
          return TCL_ERROR;
        }
    }

    ClientData getCI(Tk_Window win, ClientData masterData)
    {
      CD* r=new CD(*(CD*)masterData);
      r->tkWin=win;
      return r;
    }
    
    void displayCI(ClientData cd, Display* display, Drawable win,
                  int imageX, int imageY, int width, int height,
                  int drawableX, int drawableY)
    {
      CD& c=*(CD*)cd;
#if USE_WIN32_SURFACE
      // TkWinGetDrawableDC is an internal (ie undocumented) routine
      // for getting the DC. We need to declare something to take
      // the state parameter - two long longs should be ample here
      long long state[2];
      HDC hdc=TkWinGetDrawableDC(display, win, state);
      //HDC hdc=GetDC(Tk_GetHWND(win));
      SaveDC(hdc);
      c.csurf.surface.reset
        (new TkWinSurface
         (c.csurf, c.master,
          cairo_win32_surface_create(hdc)));
#elif defined(MAC_OSX_TK)
      NSContext nctx(win);
      c.csurf.surface.reset
        (new TkWinSurface
         (c.csurf, c.master,
          cairo_quartz_surface_create_for_cg_context(nctx.context, Tk_Width(c.tkWin), Tk_Height(c.tkWin))));
      // x offset required because MacOSX draws a thick white border around this
      cairo_surface_set_device_offset(c.csurf.surface->surface(),30,Tk_Height(c.tkWin));
      cairo_surface_set_device_scale(c.csurf.surface->surface(),1,-1);
#else
      int depth;
      Visual *visual = Tk_GetVisual(interp(), c.tkWin, "default", &depth, NULL);
      c.csurf.surface.reset
        (new TkWinSurface
         (c.csurf, c.master,
          cairo_xlib_surface_create(display, win, visual, Tk_Width(c.tkWin), Tk_Height(c.tkWin))));
        
#endif
      c.csurf.redraw(imageX,imageY,width,height);
      //cairo_surface_flush(c.canvas.surface->surface());
      // release surface prior to any context going out of scope
      c.csurf.surface->surface(NULL);
#if USE_WIN32_SURFACE
      RestoreDC(hdc,-1);
      TkWinReleaseDrawableDC(win, hdc, state);
#endif
    }


  
  void freeCI(ClientData cd,Display*) {delete (CD*)cd;}
  void deleteCI(ClientData cd) {delete (CD*)cd;}
  
  Tk_ImageType canvasImage = {
    "cairoSurface",
    createCI,
    getCI,
    displayCI,
    freeCI,
    deleteCI
  };

}

void CairoSurface::registerImage()
{
  // ensure Tk_Init is called.
  if (!Tk_MainWindow(interp())) Tk_Init(interp());
  Tk_CreateImageType(&canvasImage);
}
#endif
