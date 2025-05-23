/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef CAIRO_BASE_H
#define CAIRO_BASE_H
#if defined(CAIRO)
#include "classdesc.h"
#undef None
#undef Success
#include "arrays.h"

#include <cairo/cairo.h>
#include <vector>
#include <string>
#include <limits>
#include <memory>
#include <math.h>
#include <memory>
#include <typeinfo>

namespace ecolab
{
#ifndef M_PI
  static const float M_PI = 3.1415926535f;
#endif

  namespace cairo
  {

    /// RAII class for managing save/restore of cairo contexts
    class CairoSave
    {
      cairo_t* cairo;
      CairoSave(const CairoSave&);
      void operator=(const CairoSave&);
    public:
      CairoSave(cairo_t* cairo): cairo(cairo) {cairo_save(cairo);}
      /// restore cairo context, after which destructor becomes a nop
      void restore() {cairo_restore(cairo); cairo=NULL;}
      ~CairoSave() {if (cairo) cairo_restore(cairo);}
    };
    
    /// Container object for managing cairo_surface_t lifetimes
    class Surface
    {
      CLASSDESC_ACCESS(Surface);
    private:
      cairo_surface_t* m_surface;
      cairo_t* m_cairo;
      mutable double m_width, m_height;
      bool recomputeSizes;
      void operator=(const Surface&);
      Surface(const Surface&);

      void computeSizes() const {
        if (m_surface && recomputeSizes)
          switch (cairo_surface_get_type(m_surface))
            {
            case CAIRO_SURFACE_TYPE_IMAGE:
              m_width=cairo_image_surface_get_width(m_surface);
              m_height=cairo_image_surface_get_height(m_surface);
              break;
#if CAIRO_HAS_RECORDING_SURFACE
            case CAIRO_SURFACE_TYPE_RECORDING:
              {
                double x, y;
                cairo_recording_surface_ink_extents
                  (m_surface, &x, &y, &m_width, &m_height);
              }
              break;
#endif
            default: break;
            }
      }

    public:
      /// use one of the cairo surface construction functions, not new
      /// to create the surface object passed here. Ownership is
      /// passed in.  width & height, if supplied, give the surfaces
      /// dimensions. They are optional for image surfaces.
      Surface(cairo_surface_t* s=NULL,
              double width=-1, double height=-1): m_surface(NULL), m_cairo(NULL)
      {surface(s, width, height);}
      virtual ~Surface() {
        if (m_cairo) cairo_destroy(m_cairo);
        if (m_surface) cairo_surface_destroy(m_surface);
      }
        
      cairo_surface_t* surface() const {return m_surface;}
      /// use one of the cairo surface construction functions, not new
      /// to create the surface object passed here. Ownership is
      /// passed in.  width & height, if supplied, give the surfaces
      /// dimensions. They are optional for image surfaces.
      cairo_surface_t* surface(cairo_surface_t* s,
                 int width=-1, int height=-1) {
        if (m_cairo) cairo_destroy(m_cairo);
        if (m_surface) cairo_surface_destroy(m_surface);
        m_surface = s;
        m_cairo = s? cairo_create(s): NULL;
        m_width=width; m_height=height;
        recomputeSizes=width<0 || height<0;
        return m_surface;
      }
      
      cairo_t* cairo() const {return m_cairo;}
      virtual double width() const {computeSizes(); return m_width;}
      virtual double height() const {computeSizes(); return m_height;}
      /// if the surface is a recording surface, return y coordinate
      /// of the top, otherwise 0
      virtual double top() const {
#if CAIRO_HAS_RECORDING_SURFACE
        if (m_surface && 
            cairo_surface_get_type(m_surface)==CAIRO_SURFACE_TYPE_RECORDING)
          {
            double x, y, w, h;
            cairo_recording_surface_ink_extents(m_surface, &x, &y, &w, &h);
            return y;
          }
        else
#endif
          return 0;
      }
      /// if the surface is a recording surface, return x coordinate
      /// of the left hand side, otherwise 0
      virtual double left() const {
#if CAIRO_HAS_RECORDING_SURFACE
        if (m_surface && 
            cairo_surface_get_type(m_surface)==CAIRO_SURFACE_TYPE_RECORDING)
          {
            double x, y, w, h;
            cairo_recording_surface_ink_extents(m_surface, &x, &y, &w, &h);
            return x;
          }
        else
#endif
          return 0;
      }
      /// clear the surface, returing it to its initial blank state.
      /// this is not the same as painting a white rectangle, as
      /// transparent surfaces are a bt different
      virtual void clear() {
        // TODO not sure if this should be pure virtual, or whether a
        // sensible default implementation can be provided.
      }
      /// Renders part of surface to screen
      /// @param x,y offset withi cairoSurface with respect to drawable
      /// @param width, height of imaged area
      // TODO - can we obsolete this method?
      virtual void blit(unsigned x, unsigned y, unsigned width, unsigned height) {}
      /// Renders surface to screen
      virtual void blit() {}
      /// resizes Tk photo surface - noop if not relevant
      virtual void resize(size_t width, size_t height) {}
      /// request redraw of this surface on a Tk canvas (if Tk canvas item)
      virtual void requestRedraw() {}
    };

    // fix the surfacePtr type so that objects and headers are consistent
#if defined(__cplusplus) && __cplusplus>=201103L
    typedef std::shared_ptr<Surface> SurfacePtr;
#else
    typedef std::tr1::shared_ptr<Surface> SurfacePtr;
#endif
    
    /// RAII object wrapping a cairo_path_t, along with its transformation
    class Path
    {
      cairo_path_t* m_path;
      cairo_matrix_t m_transformation;
      Path(const Path&);
      void operator=(const Path&);
    public:
      Path(cairo_t* cairo): m_path(cairo_copy_path(cairo))
      {cairo_get_matrix(cairo, &m_transformation);}
      ~Path() {cairo_path_destroy(m_path);}
      void appendToCurrent(cairo_t* cairo) {
        // apply saved transformation along with path
        cairo_save(cairo);
        cairo_set_matrix(cairo,&m_transformation);
        cairo_append_path(cairo,m_path);
        cairo_restore(cairo);
      }
    };


  }
}

#define CLASSDESC_pack___ecolab__cairo__Surface
#define CLASSDESC_unpack___ecolab__cairo__Surface

namespace classdesc_access
{
  template <> struct access_pack<ecolab::cairo::Surface>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
  template <> struct access_unpack<ecolab::cairo::Surface>:
    public classdesc::NullDescriptor<classdesc::pack_t> {};
}

#include "cairo_base.cd"
#endif

#ifdef TK
#include "tk_cairo.h"
#endif

#endif
