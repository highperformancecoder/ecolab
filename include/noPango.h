/*
  @copyright Russell Standish 2021
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef ECOLAB_NOPANGO_H
#define ECOLAB_NOPANGO_H

#include <cairo.h>
#include <string>

namespace ecolab
{
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
    static const char *defaultFamily;
    static double scaleFactor;
    double angle; // angle the text  
    Pango(cairo_t* cairo): 
      cairo(cairo), fontSize(10), angle(0) {}
    void setMarkup(const std::string& markup) {
      this->markup=markup;
      cairo_text_extents(cairo,markup.c_str(),&bbox);
      
    }
    void setText(const std::string& markup) {setMarkup(markup);}
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
}

#endif
