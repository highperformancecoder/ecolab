/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#include "classdesc.h"
#include "cairo_base.h"
#include "pango.h"
#include "ecolab_epilogue.h"

const char *ecolab::Pango::defaultFamily=NULL;

#if defined(CAIRO) && defined(TK)

#if CAIRO_VERSION_MAJOR <= 1 && CAIRO_VERSION_MINOR < 10
//#error "Cairo 1.10.x minimum required"
// fake the in_clip call - hopefully this doesn't affect functionality too much
bool cairo_in_clip(cairo_t*,double,double) {return true;}
#endif

using namespace std;

namespace
{
  // squared distance between two points
  inline double dist(double x0, double y0, double x1, double y1)
  {
    double dx=x0-x1, dy=y0-y1;
    return dx*dx+dy*dy;
  }

  
  // squared distance between a point (x,y) and a line segment ab
  inline double dist(double x, double y, double ax, double ay, double bx, double by)
  {
    double dax = x-ax, day = y-ay, dabx = bx-ax, daby = by-ay;
    double dot = dax*dabx + day*daby; // inner product
    return sqrt(dax*dax + day*day - dot*dot / (dabx*dabx + daby*daby));
  }

//  // squared distance between a rectangle and a point
//  double dist(const cairo_rectangle_t& r, double x, double y)
//  {
//    double rx1 = r.x+r.width, ry1=r.y+r.height;
//    if (x < r.x)
//      if (y < r.y)
//        return dist(x,y,r.x,r.y);
//      else if (y > ry1)
//        return dist(x,y,r.x,ry1);
//      else
//        return dist(x,y,r.x,r.y,r.x,ry1);
//    else if (x > rx1)
//      if (y < r.y)
//        return dist(x,y,rx1,r.y);
//      else if (y > ry1)
//        return dist(x,y,rx1,ry1);
//      else
//        return dist(x,y,rx1,r.y,rx1,ry1);
//    else
//      if (y > r.y)
//        return dist(x,y,r.x,r.y,rx1,r.y);
//      else if (y < ry1)
//        return dist(x,y,r.x,ry1,rx1,ry1);
//      else
//        return 0;
//  }

  class ClipRegion
  {
    cairo_rectangle_list_t* rectangles;
    cairo_rectangle_t boundingBox;
  public:
    ClipRegion(cairo_t* cairo) {
      rectangles = cairo_copy_clip_rectangle_list(cairo);
      cairo_clip_extents(cairo, &boundingBox.x, &boundingBox.y, 
                         &boundingBox.width, &boundingBox.height);
      boundingBox.width-=boundingBox.x;
      boundingBox.height-=boundingBox.y;
      assert(boundingBox.width>=0 && boundingBox.height>=0);
    }
    ~ClipRegion() {
      cairo_rectangle_list_destroy(rectangles);
    }
  };
  
}

namespace ecolab
{
  double Pango::scaleFactor=1;
  
  namespace cairo
  {
    void CairoImage::resize(size_t width, size_t height)
    {
      if (cairoSurface) cairoSurface->resize(width, height);
    }

    void CairoImage::redrawIfSurfaceTooSmall()
    {
      array<double> clipBox=boundingBox();
      double width=clipBox[2]-clipBox[0], height=clipBox[3]-clipBox[1];
      // handle -ve coordinates
      if (clipBox[0]<0) width+=-clipBox[0];
      if (clipBox[1]<0) height+=-clipBox[1];
      if (width>cairoSurface->width() || height>cairoSurface->height())
        {
          resize(2*width+1, 2*height+1); //increase surface size and redraw
          try {draw(); }
          catch (const std::exception& e) 
            {cerr<<"illegal exception caught in draw()"<<e.what()<<endl;}
          catch (...) {cerr<<"illegal exception caught in draw()";}
        }
     }

    void CairoImage::initMatrix()
    {
      if (cairoSurface)
        {
          cairo_t* cairo = cairoSurface->cairo();
          cairo_identity_matrix(cairo);
          cairo_scale(cairo, m_scale, m_scale);
          cairo_rotate(cairo, rotation);
          cairo_scale(cairo, xScale, yScale);
        }
    }

    void CairoImage::attachToImage(const std::string& imgName)
    {
//      Tk_PhotoHandle photo=Tk_FindPhoto(interp(),imgName.c_str());
//      if (photo)
//        cairoSurface.reset(new TkPhotoSurface(photo));
//      else
//        throw error("image %s not found", imgName.c_str());
    }

   double CairoImage::distanceFrom(double x, double y) const
    {
      if (cairoSurface)
        {
          cairo_t *cairo = cairoSurface->cairo();
          // coordinates are relative to the image centre point - need
          // to translate to get it into device coordinates
          x+=0.5*cairoSurface->width();
          y+=0.5*cairoSurface->height();
          cairo_device_to_user(cairo,&x,&y);
          if (cairo_in_clip(cairo,x,y))
            {
              return 0;
            }

          // Try to find distance to clip region by a Newton-Raphson method
          double dr=500, radius=1000; // max distance possible
          while (dr>1)
            {
              cairo_save(cairo);
              cairo_arc(cairo,x,y,radius,0,2*M_PI);
              cairo_clip(cairo);
              double x0,x1,y0,y1;
              cairo_clip_extents(cairo,&x0,&y0,&x1,&y1);
              cairo_restore(cairo);
              if (x0!=x1 || y0!=y1 ||
                  cairo_in_clip(cairo,x0,y0) || cairo_in_clip(cairo,x0,y1) ||
                  cairo_in_clip(cairo,x1,y0) || cairo_in_clip(cairo,x1,y1))
                  radius-=dr; // still some clip left
              else
                radius+=dr; // clipped.
              dr*=0.5;
            }
          double zero=0;
          cairo_user_to_device_distance(cairo,&radius,&zero);
          return radius;
        }
      return std::numeric_limits<double>::max();
    }

    array<double> CairoImage::boundingBox()
    {
      array_ns::array<double> bbox(4);
#ifdef CAIRO_HAS_RECORDING_SURFACE
      SurfacePtr tempSurf
        (new Surface
         (cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA,NULL)));
      cairoSurface.swap(tempSurf);
      try {draw();}
      catch (const std::exception& e) 
        {cerr<<"illegal exception caught in draw()"<<e.what()<<endl;}
      catch (...) {cerr<<"illegal exception caught in draw()";}
      cairo_recording_surface_ink_extents(cairoSurface->surface(),
                                          &bbox[0],&bbox[1],&bbox[2],&bbox[3]);
      bbox[2]+=bbox[0];
      bbox[3]+=bbox[1];
      cairoSurface.swap(tempSurf);
#else
      throw error("Please upgrade your cairo to a version implementing recording surfaces");
#endif
      return bbox;
    }

    int CairoImage::inClip(double x0, double y0, double x1, double y1) const
    {       
      if (cairoSurface)
        {
          cairo_t *cairo = cairoSurface->cairo();
          // convert to user coordinates and scale
          x0+=0.5*cairoSurface->width();
          y0+=0.5*cairoSurface->height();
          x1+=0.5*cairoSurface->width();
          y1+=0.5*cairoSurface->height();
          cairo_device_to_user(cairo,&x0,&y0);
          cairo_device_to_user(cairo,&x1,&y1);
          // rectangle and previous clip
          int nCornersIn=cairo_in_clip(cairo,x0,y0);
          nCornersIn+=cairo_in_clip(cairo,x0,y1);
          nCornersIn+=cairo_in_clip(cairo,x1,y0);
          nCornersIn+=cairo_in_clip(cairo,x1,y1);
          if (nCornersIn==0) return -1; // all out
          if (nCornersIn==4) return 1; // all in
        }
      return 0;
    }
  }
}

#endif
