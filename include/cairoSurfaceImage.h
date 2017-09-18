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
    virtual ~CairoSurface() {}
    
    /** 
        registers cairoSurface as an image type with the TCL interpreter.
        Normally doesn't need to be called by the user, as the Tkinit
        command will do it. 
    */
    static void registerImage();
  };
}

#ifdef _CLASSDESC
#pragma omit pack CairoSurface
#pragma omit unpack CairoSurface
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
#endif
#endif
