/*
  @copyright Russell Standish 2017
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef CAIROSURFACEIMAGE_H
#define CAIROSURFACEIMAGE_H
#if defined(CAIRO) && defined(TK)
#include "cairo_base.h"

namespace ecolab
{
  /** A base class used for creating a custom image object. To use,
      subclass from this, reimplementing the redraw method, then in
      TCL, create an image with the type "cairoSurface", passing in
      the TCL_obj name as the argument of -surface, eg
      
      image create cairoSurface -surface minsky.canvas
 */

  struct CairoSurface
  {
    Exclude<cairo::SurfacePtr> surface;
    virtual void redraw(int x0, int y0, int width, int height)=0;
    virtual void redrawWithBounds() {redraw(-1e9,-1e9,2e9,2e9);} //TODO better name for this?
    virtual ~CairoSurface() {}
    // arrange a callback with the drawing time in seconds
    virtual void reportDrawTime(double) {}
    
    /** 
        registers cairoSurface as an image type with the TCL interpreter.
        Normally doesn't need to be called by the user, as the Tkinit
        command will do it. 
    */
    static void registerImage();
    /// export to a file, using a surface factory \a s
    cairo::SurfacePtr vectorRender
    (const char* filename, cairo_surface_t* (*s)(const char *,double,double));

    /// render to a postscript file
    void renderToPS(const char* filename);
    /// render to a PDF file
    void renderToPDF(const char* filename);
    /// render to an SVG file
    void renderToSVG(const char* filename);
    /// render to a PNG image file
    void renderToPNG(const char* filename);
    /// render canvas to a EMF file. Windows only.
    void renderToEMF(const char* filename);
  };
}

#ifdef _CLASSDESC
#pragma omit pack ecolab::CairoSurface
#pragma omit unpack ecolab::CairoSurface
#endif
namespace classdesc_access
{
  template <>
  struct access_pack<ecolab::CairoSurface>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <>
  struct access_unpack<ecolab::CairoSurface>:
    public classdesc::NullDescriptor<classdesc::unpack_t> {};
}

namespace classdesc
{
  template <> struct tn<ecolab::CairoSurface>
  {
    static string name() {return "ecolab::CairoSurface";}
  };
}

#include "cairoSurfaceImage.cd"
#endif
#endif
