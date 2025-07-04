/*
  @copyright Russell Standish 2017
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef CAIROSURFACEIMAGE_H
#define CAIROSURFACEIMAGE_H
#if defined(CAIRO)
#include "cairo_base.h"

namespace ecolab
{
  /** A base class used for creating a custom image object. To use,
      subclass from this, reimplementing the redraw method, then in
      Python/tkinter, create an image with the type "cairoSurface", passing in
      the python object name as the argument of -surface, eg
      
      tk.eval('image create cairoSurface -surface module object')

 */

  struct CairoSurfaceRedraw
  {
    cairo::SurfacePtr surface;
    /// @return true if something drawn
    virtual bool redraw(int x0, int y0, int width, int height)=0;
    virtual void redrawWithBounds() {redraw(-1e6,-1e6,2e6,2e6);} //TODO better name for this?
    /// increase output resolution of pixmap surfaces by this factor
    double m_resolutionScaleFactor=1.0;
    /// export to a file, using a surface factory \a s
    cairo::SurfacePtr vectorRender
    (const char* filename, cairo_surface_t* (*s)(const char *,double,double));
  };

  struct CairoSurface: public classdesc::Exclude<CairoSurfaceRedraw>
  {
    virtual ~CairoSurface() {}
    // arrange a callback with the drawing time in seconds
    virtual void reportDrawTime(double) {}
    
    /** 
        registers cairoSurface as an image type with the TCL interpreter.
        Normally doesn't need to be called by the user, as the Tkinit
        command will do it. 
    */
    static void registerImage();

    /// increase output resolution of pixmap surfaces by this factor
    double resolutionScaleFactor() const {return m_resolutionScaleFactor;}
    double resolutionScaleFactor(double rsf) {return m_resolutionScaleFactor=rsf;}
    /// render to a postscript file
    void renderToPS(const std::string& filename);
    /// render to a PDF file
    void renderToPDF(const std::string& filename);
    /// render to an SVG file
    void renderToSVG(const std::string& filename);
    /// render to a PNG image file
    void renderToPNG(const std::string& filename);
    /// render canvas to a EMF file. Windows only.
    void renderToEMF(const std::string& filename);
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
  template <> struct tn<_cairo_surface>
  {
    static string name() {return "_cairo_surface";}
  };
  template <> struct tn<_cairo>
  {
    static string name() {return "_cairo";}
  };
}

#include "cairoSurfaceImage.cd"
#endif
#endif
