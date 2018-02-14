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
#else
#include <cairo.h>
#endif

#include <string>
#include <assert.h>

namespace ecolab
{
#ifdef PANGO
  class Pango
  {
    cairo_t* cairo;
    PangoLayout* layout;
    PangoFontDescription* fd;
    PangoRectangle bbox;
    void operator=(const Pango&);
    Pango(const Pango&);
  public:
    double angle; // angle the text
    static const char *defaultFamily;
    Pango(cairo_t* cairo): 
      cairo(cairo), layout(pango_cairo_create_layout(cairo)), angle(0) 
    {
      fd=pango_font_description_copy(pango_layout_get_font_description(layout));
      if (!fd) // if NULL, we must get it from the context
        {
          fd=pango_font_description_copy
            (pango_context_get_font_description(pango_layout_get_context(layout)));
        }
      if (defaultFamily)
        pango_font_description_set_family(fd,defaultFamily);
      pango_layout_set_font_description(layout, fd); //asume ownership not passe
    }
    ~Pango() {pango_font_description_free(fd); g_object_unref(layout); }
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
      if (gint(sz*PANGO_SCALE)<=0) return;
      pango_font_description_set_size(fd, gint(sz*PANGO_SCALE));
      pango_layout_set_font_description(layout, fd); //asume ownership not passed?
    }
    void setFontFamily(const char* family) {
      pango_font_description_set_family(fd,family);
      pango_layout_set_font_description(layout, fd); //asume ownership not passe 
  }
    double getFontSize() const {
      return pango_font_description_get_size(fd)/double(PANGO_SCALE);
    }
    void show() {
//      cairo_save(cairo);
//      cairo_identity_matrix(cairo);
//      cairo_rotate(cairo, angle);
//      cairo_rectangle(cairo,left(),top(),width(),height());
//      cairo_stroke(cairo);
//      cairo_restore(cairo);

      cairo_save(cairo);
      cairo_identity_matrix(cairo);
      cairo_rotate(cairo, angle);
      cairo_rel_move_to(cairo,left(),top());
      pango_cairo_update_layout(cairo, layout);
      pango_cairo_show_layout(cairo, layout);
      cairo_restore(cairo);
    }
    /// return index into the markup string corresponding to \a x from
    /// the start of the string in screen coordinates
    std::string::size_type posToIdx(double x) const {
      int r=0, graphemeOffset;
      pango_layout_xy_to_index(layout, PANGO_SCALE*x, 0, &r, &graphemeOffset);
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
      return double(pos)/PANGO_SCALE;
    }
    
    /// width of rendered text
    double width() const {return double(bbox.width)/PANGO_SCALE;}
    /// height of rendered text
    double height() const {return double(bbox.height)/PANGO_SCALE;}
    /// x-coordinate of left hand side of the rendered text
    double left() const {return double(bbox.x)/PANGO_SCALE;}
    /// y-coordinate of the top of the rendered text
    double top() const {return double(bbox.y)/PANGO_SCALE;}
 };
#else // fall back to basic Cairo text handling
  class Pango
  {
    cairo_t* cairo;
    std::string markup;
    cairo_text_extents_t bbox;
    double fontSize; // default according to cairo documentation
    // make this non-copiable so that it behaves the same as the real Pango one
    void operator=(const Pango&);
    Pango(const Pango&);
  public:
    double angle; // angle the text  
    Pango(cairo_t* cairo): 
      cairo(cairo), fontSize(10), angle(0) {}
    void setMarkup(const std::string& markup) {
      this->markup=markup;
      cairo_text_extents(cairo,markup.c_str(),&bbox);
      
    }
    void setFontSize(unsigned sz) {
      fontSize=sz;
      cairo_set_font_size(cairo, sz);
    }
    double getFontSize() const {
      return fontSize;
    }
    void show() {
      cairo_save(cairo);
      cairo_identity_matrix(cairo);
      cairo_rotate(cairo, angle);
      cairo_rel_move_to(cairo,left(),-top());
      cairo_show_text(cairo, markup.c_str());
      cairo_restore(cairo);
    }
    /// return index into the markup string corresponding to \a x from
    /// the start of the string in screen coordinates
    std::string::size_type posToIdx(double x) const {
      cairo_text_extents_t bbox;
      bbox.width=0;
      std::string::size_type i=0;
      for (;i<markup.size() && bbox.width<x; ++i)
        cairo_text_extents(cairo,markup.substr(0,i).c_str(),&bbox);
      return (i==0 || markup[i]==' ')?i: i-1; // a bit hacky, but kind of works
    }
    /// return distance along string character \a idx is rendered at
    double idxToPos(std::string::size_type idx) const {
      cairo_text_extents_t bbox;
      cairo_text_extents(cairo,markup.substr(0,idx).c_str(),&bbox);
      return bbox.width;
    }
      /// width of rendered text
    double width() const {return bbox.width;}
    /// height of rendered text
    double height() const {return bbox.height;}
    /// x-coordinate of left hand side of the rendered text
    double left() const {return bbox.x_bearing;}
    /// y-coordinate of the top of the rendered text
    double top() const {return bbox.y_bearing;}
 };
#endif
}

#endif
