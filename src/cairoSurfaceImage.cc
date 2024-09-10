/*
  @copyright Russell Standish 2017
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#if defined(CAIRO) && defined(TK)
#include "cairoSurfaceImage.h"
#include "tcl++.h"
#include "pango.h"
#include "pythonBuffer.h"

#include "ecolab_epilogue.h"
#if defined(CAIRO_HAS_WIN32_SURFACE) && !defined(__CYGWIN__)
#define USE_WIN32_SURFACE
#endif

#include <cairo/cairo-ps.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>

#ifdef _WIN32
#undef Realloc
#include <windows.h>
#include <wingdi.h>
#ifdef USE_WIN32_SURFACE
#include <cairo/cairo-win32.h>
// undocumented internal function for extracting the HDC from a Drawable
extern "C" HDC TkWinGetDrawableDC(Display*, Drawable, void*);
extern "C" HDC TkWinReleaseDrawableDC(Drawable, HDC, void*);
#endif
#endif


#if defined(CAIRO_HAS_XLIB_SURFACE) && !defined(MAC_OSX_TK)
#include <cairo/cairo-xlib.h>
#endif

#if defined(MAC_OSX_TK)
#include <Carbon/Carbon.h>
#include <cairo/cairo-quartz.h>
#include "getContext.h"
#endif

#if TK_MAJOR_VERSION==8 && TK_MINOR_VERSION < 6
#define CONST86
#endif

using namespace ecolab;
using namespace std;

namespace
{
    struct TkWinSurface: public ecolab::cairo::Surface
    {
      CairoSurface& csurf;
      Tk_ImageMaster imageMaster;
      int m_width, m_height;
      TkWinSurface(CairoSurface& csurf, Tk_ImageMaster imageMaster, cairo_surface_t* surf, int width, int height):
        cairo::Surface(surf), csurf(csurf),  imageMaster(imageMaster), m_width(width), m_height(height) {}
      void requestRedraw() {
        Tk_ImageChanged(imageMaster,left(),top(),m_width,m_height,m_width,m_height);
      }
      void blit() {cairo_surface_flush(surface());}
      double width() const override {return m_width;}
      double height() const override {return m_height;}
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
    int createCI(Tcl_Interp* interp, CONST86 char* name, int objc, Tcl_Obj *const objv[],
                 CONST86 Tk_ImageType* typePtr, Tk_ImageMaster master, ClientData *masterData)
    {
      try
        {
          TCL_args args(objc, objv);
          std::string module=args, object=args;
          auto r=registries().find(module);
          if (r==registries().end() || !r->second) throw std::runtime_error("Module: "+module+" not found");
          auto rp=r->second->find(object);
          if (rp==r->second->end()) throw std::runtime_error("Object: "+object+" not found in "+module);
          if (CairoSurface* csurf=
              dynamic_cast<CairoSurface*>(const_cast<classdesc::object*>(rp->second->getClassdescObject())))
            {
              *masterData=new CD(0,master,*csurf);
              Tk_ImageChanged(master,0,0,500,500,500,500);
              return TCL_OK;
            }
          throw std::runtime_error("Not a CairoSurface");
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
      clock_t t0=clock();
      CD& c=*(CD*)cd;
      width=min(width,Tk_Width(c.tkWin));
      height=min(height,Tk_Height(c.tkWin));
#ifdef USE_WIN32_SURFACE
      // TkWinGetDrawableDC is an internal (ie undocumented) routine
      // for getting the DC. We need to declare something to take
      // the state parameter - two long longs should be ample here
      long long state[2];
      HDC hdc=TkWinGetDrawableDC(display, win, state);
      if (!hdc) return;
      //HDC hdc=GetDC(Tk_GetHWND(win));
      int savedDC=SaveDC(hdc);
      c.csurf.surface.reset
        (new TkWinSurface
         (c.csurf, c.master,
          cairo_win32_surface_create(hdc),Tk_Width(c.tkWin)-2, Tk_Height(c.tkWin)-2)));
#elif defined(MAC_OSX_TK)
      // calculate the offset of the window within it's toplevel
      int xoffs=0, yoffs=0;
      for (Tk_Window w=c.tkWin; !Tk_IsTopLevel(w); w=Tk_Parent(w))
        {
          xoffs+=Tk_X(w);
          yoffs+=Tk_Y(w);
        }
      
      NSContext nctx(win, xoffs, yoffs);
      c.csurf.surface.reset
        (new TkWinSurface
         (c.csurf, c.master,
          cairo_quartz_surface_create_for_cg_context(nctx.context, Tk_Width(c.tkWin), Tk_Height(c.tkWin)),Tk_Width(c.tkWin)-2, Tk_Height(c.tkWin)-2)));
#else
      int depth;
      Visual *visual = Tk_GetVisual(interp(), c.tkWin, "default", &depth, NULL);
      c.csurf.surface.reset
         (new TkWinSurface
         (c.csurf, c.master,
          cairo_xlib_surface_create(display, win, visual, Tk_Width(c.tkWin), Tk_Height(c.tkWin)),Tk_Width(c.tkWin)-2, Tk_Height(c.tkWin)-2));
      // why subtract 2?
        
#endif
      try
        {
          c.csurf.redraw(imageX,imageY,width,height);
        }
      catch (const std::exception& ex)
        {
          // display message in scary red letters
          cairo_t* cairo=c.csurf.surface->cairo();
          cairo_reset_clip(cairo);
          cairo_identity_matrix(cairo);
          cairo_set_source_rgba(cairo,1,0,0,0.5);
          Pango p(cairo);
          p.setFontSize(24);
          p.setText(string("Error: ")+ex.what());
          cairo_move_to(cairo,imageX+0.5*(width-p.width()),imageY+0.5*(height-p.height()));
          p.show();
        }
      catch (...)
        {/* not much you can do about exceptions at this point */}
      cairo_surface_flush(c.csurf.surface->surface());
#ifdef USE_WIN32_SURFACE
      if (savedDC)
        RestoreDC(hdc,savedDC);
      TkWinReleaseDrawableDC(win, hdc, state);
#endif
      c.csurf.reportDrawTime(double(clock()-t0)/CLOCKS_PER_SEC);
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
  Tk_CreateImageType(&canvasImage);
}

cairo::SurfacePtr CairoSurface::vectorRender(const char* filename, cairo_surface_t* (*s)(const char *,double,double))
{
  cairo::SurfacePtr tmp(new cairo::Surface(cairo_recording_surface_create
                                           (CAIRO_CONTENT_COLOR_ALPHA,NULL)));
  surface.swap(tmp);
  redrawWithBounds();
  double left=surface->left(), top=surface->top();
  surface->surface
    (s(filename, surface->width()*resolutionScaleFactor, surface->height()*resolutionScaleFactor));
  if (s==cairo_ps_surface_create)
    cairo_ps_surface_set_eps(surface->surface(),true);
  cairo_surface_set_device_offset(surface->surface(), -left, -top);
  cairo_surface_set_device_scale(surface->surface(), resolutionScaleFactor, resolutionScaleFactor);
  redrawWithBounds();
  cairo_surface_flush(surface->surface());
  surface.swap(tmp);
  cairo_status_t status=cairo_surface_status(tmp->surface());
  if (status!=CAIRO_STATUS_SUCCESS)
    throw error("cairo rendering error: %s",cairo_status_to_string(status));
  return tmp;
}
  
void CairoSurface::renderToPS(const string& filename)
{vectorRender(filename.c_str(),cairo_ps_surface_create);}

void CairoSurface::renderToPDF(const string& filename)
{vectorRender(filename.c_str(),cairo_pdf_surface_create);}

void CairoSurface::renderToSVG(const string& filename)
{vectorRender(filename.c_str(),cairo_svg_surface_create);}

namespace
{
  cairo_surface_t* pngDummy(const char*,double width,double height)
  {return cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);}
}
  
void CairoSurface::renderToPNG(const string& filename)
{
  cairo::SurfacePtr tmp=vectorRender(filename.c_str(),pngDummy);
  cairo_surface_write_to_png(tmp->surface(),filename.c_str());
}

#ifdef _WIN32
namespace
{
  struct SurfAndDC
  {
    HDC hdc;
    cairo_surface_t* surf;
  };
    
  cairo_user_data_key_t closeKey;
  void closeFile(void *x)
  {
      SurfAndDC* s(static_cast<SurfAndDC*>(x));
      cairo_surface_flush(s->surf);
      // nb the Delete... function deletes the handle created by Close...
      DeleteEnhMetaFile(CloseEnhMetaFile(s->hdc));
      DeleteDC(s->hdc);
      delete s;
  }

  cairo_surface_t* createEMFSurface(const char* filename, double width, double height)
  {
    HDC hdc=CreateEnhMetaFileA(nullptr,filename,nullptr,"Minsky\0");
    cairo_surface_t* surf=cairo_win32_surface_create_with_format(hdc,CAIRO_FORMAT_ARGB32);
    // initialise the image background 
    cairo_t* cairo=cairo_create(surf);
    // extend the background a bit beyond the bounding box to allow
    // for additional black stuff inserted by GDI
    cairo_rectangle(cairo,0,0,width+50,height+50);
    cairo_set_source_rgb(cairo,1,1,1);
    cairo_clip_preserve(cairo);
    cairo_fill(cairo);
    cairo_destroy(cairo);
    // set up a callback to flush and close the EMF file
    cairo_surface_set_user_data(surf,&closeKey,new SurfAndDC{hdc,surf},closeFile);
    
    return surf;
  }
}

void CairoSurface::renderToEMF(const string& filename)
{vectorRender(filename.c_str(),createEMFSurface);}

#else

void CairoSurface::renderToEMF(const string& filename)
{throw std::runtime_error("EMF functionality only available on Windows");}

#endif

#endif
