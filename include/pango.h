/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

/*
  Utility class for wrapping Pango over cairo functionality
 
  Pango text is in the Pango basic markup
*/
#ifndef ECOLAB_PANGO_H
#define ECOLAB_PANGO_H

#ifdef PANGO
#include <pango/pangocairo.h>
#include <cairo_base.h>
#include <error.h>

#include <string>
#include <assert.h>

namespace ecolab
{
  class Pango
  {
    cairo_t* cairo;
    PangoLayout* layout;
    struct FontDescription
    {
      PangoFontDescription* fd;
      FontDescription(PangoLayout* l) {
        fd=pango_font_description_copy(pango_layout_get_font_description(l));
        if (!fd) // if NULL, we must get it from the context
          {
            fd=pango_font_description_copy
              (pango_context_get_font_description(pango_layout_get_context(l)));
          }
      }
      ~FontDescription() {
        pango_font_description_free(fd);
      }
      operator PangoFontDescription*() {return fd;}
    };
    PangoRectangle bbox;
    void operator=(const Pango&);
    Pango(const Pango&);
    void throwOnError() const {
      cairo_status_t status=cairo_status(cairo);
      if (status!=CAIRO_STATUS_SUCCESS)
        throw error(cairo_status_to_string(status));
    }
    double scale;
  public:
    double angle; // angle the text
    static const char *defaultFamily;
    /// an additional font size scale factor to help rendering on Windows high DPI displays.
    static double scaleFactor;
    Pango(cairo_t* cairo): 
      cairo(cairo), layout(pango_cairo_create_layout(cairo)), scale(scaleFactor), angle(0) 
    {
      PangoRectangle tmp={0,0,0,0};
      bbox=tmp;
      FontDescription fd(layout);
      if (defaultFamily)
        pango_font_description_set_family(fd,defaultFamily);
      pango_layout_set_font_description(layout, fd); //assume ownership not passed
    }
    ~Pango() {
      if (layout) g_object_unref(layout);
    }
#if defined(__cplusplus) && __cplusplus>=201103L
    Pango(Pango&& x): cairo(x.cairo), layout(x.layout),
                      bbox(x.bbox), angle(x.angle)
    {x.layout=nullptr;}
#endif
    /// set text to be displayed in pango markup language
    void setMarkup(const std::string& markup) {
      pango_layout_set_markup(layout, markup.c_str(), -1);
      pango_layout_get_extents(layout,0,&bbox);
    }
    /// set raw text to be displayed (no markup interpretation)
    void setText(const std::string& markup) {
      pango_layout_set_attributes(layout,NULL);
      pango_layout_set_text(layout, markup.c_str(), -1);
      pango_layout_get_extents(layout,0,&bbox);
    }
    void setFontSize(double sz) {
      if (!isfinite(sz)) return; // cowardly refuse to set a non real valued font size
      scale*=scaleFactor*sz/getFontSize();
    }
    void setFontFamily(const char* family) {
      FontDescription fd(layout);
      pango_font_description_set_family(fd,family);
      pango_layout_set_font_description(layout, fd); //asume ownership not passed
  }
    double getFontSize() const {
      FontDescription fd(layout);
      return scale*pango_font_description_get_size(fd)/double(PANGO_SCALE);
    }
    void show() {
//      cairo_save(cairo);
//      cairo_identity_matrix(cairo);
//      cairo_rotate(cairo, angle);
//      cairo_rectangle(cairo,left(),top(),width(),height());
//      cairo_stroke(cairo);
//      cairo_restore(cairo);

      cairo::CairoSave cs(cairo);
      cairo_rotate(cairo, angle);
      cairo_rel_move_to(cairo,left(),top());
      cairo_scale(cairo,scale,scale);
      pango_cairo_update_layout(cairo, layout);
      throwOnError();
      pango_cairo_show_layout(cairo, layout);
      throwOnError();
    }
    /// return index into the markup string corresponding to \a x from
    /// the start of the string in screen coordinates
    std::string::size_type posToIdx(double x) const {
      int r=0, graphemeOffset;
      pango_layout_xy_to_index(layout, PANGO_SCALE*(x/scale), 0, &r, &graphemeOffset);
      // nb ignoring offset into multibyte characters
      return r;
    }
    /// return distance along string character \a idx is rendered at
    /// idx <= markup.length()
    double idxToPos(std::string::size_type idx) const {
      int line=0, pos=0;
      pango_layout_index_to_line_x(layout, idx, false, &line, &pos);
      // nb ignoring multiline possibilities
      assert(line==0);
      return scale*double(pos)/PANGO_SCALE;
    }
    
    /// width of rendered text
    double width() const {return scale*double(bbox.width)/PANGO_SCALE;}
    /// height of rendered text
    double height() const {return scale*double(bbox.height)/PANGO_SCALE;}
    /// x-coordinate of left hand side of the rendered text
    double left() const {return scale*double(bbox.x)/PANGO_SCALE;}
    /// y-coordinate of the top of the rendered text
    double top() const {return scale*double(bbox.y)/PANGO_SCALE;}
 };
}
#else // fall back to basic Cairo text handling
#include "noPango.h"
#endif

#endif
