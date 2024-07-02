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
#include "arrays.h"

#include <cairo/cairo.h>
#ifdef TK
#include <tk.h>
#endif
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
      double width() const {computeSizes(); return m_width;}
      double height() const {computeSizes(); return m_height;}
      /// if the surface is a recording surface, return y coordinate
      /// of the top, otherwise 0
      double top() const {
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
      double left() const {
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
    
#ifdef TK
    struct PhotoImageBlock: public Tk_PhotoImageBlock
    {
      PhotoImageBlock(): transparency(true) {}
      PhotoImageBlock(int x, int y, bool transparency): 
        transparency(transparency)
      { 
        pixelPtr=NULL;
        width=x;
        height=y;
        cairo_format_t format = transparency? 
          CAIRO_FORMAT_ARGB32: CAIRO_FORMAT_RGB24;
        pitch = cairo_format_stride_for_width(format, width); 
        pixelSize=4; 
        for (int i=0; i<3; ++i) offset[i]=2-i;
        offset[3]=3;
      }
      bool transparency;
    };
        
    class TkPhotoSurface: public Surface
    {
      Tk_PhotoHandle photo;
      std::vector<unsigned char> imageData;
      PhotoImageBlock imageBlock;
    public:
      /// if true, then blits do not clear previous photo contents
      bool compositing;

      int width() const {return imageBlock.width;}
      int height() const {return imageBlock.height;}
      void clear() {
        if (imageBlock.transparency)
          // make it transparent
          memset(imageData.data(),0,imageData.size());
        else
          // make it white
          memset(imageData.data(),255,imageData.size());
      }
      TkPhotoSurface(Tk_PhotoHandle photo, bool transparency=true): 
        photo(photo), compositing(false)
      {
        init(transparency);
      }

      void init(bool transparency=true)
      {
        int width, height;
        Tk_PhotoGetSize(photo, &width, &height);
        imageBlock = PhotoImageBlock(width, height, transparency);
        imageData.resize(size_t(imageBlock.pitch) * imageBlock.height);
        imageBlock.pixelPtr = imageData.data();

        surface( cairo_image_surface_create_for_data
                 (imageData.data(), transparency? 
                  CAIRO_FORMAT_ARGB32: CAIRO_FORMAT_RGB24,
                  width, height, imageBlock.pitch));
      }

      void resize(size_t width, size_t height)
      {
//#if TK_MAJOR_VERSION == 8 && TK_MINOR_VERSION < 5
//        Tk_PhotoSetSize(photo, width, height);
//#else
//        Tk_PhotoSetSize(interp(), photo, width, height);
//#endif
//        init(imageBlock.transparency);
      }

      /// copy the cairo output into the TkPhoto
      void blit(unsigned x, unsigned y, unsigned width, unsigned height) 
      {
//        imageBlock.pixelPtr=&imageData[x*imageBlock.pixelSize + y*imageBlock.pitch];
//        if (width != unsigned(imageBlock.width) || 
//            height != unsigned(imageBlock.height))
//#if TK_MAJOR_VERSION == 8 && TK_MINOR_VERSION < 5
//          Tk_PhotoSetSize(photo, width, height);
//        Tk_PhotoPutBlock
//          (photo, &imageBlock, 0, 0, width, height, 
//          compositing? TK_PHOTO_COMPOSITE_OVERLAY:TK_PHOTO_COMPOSITE_SET);
//#else
//          Tk_PhotoSetSize(interp(), photo, width, height);
//        Tk_PhotoPutBlock(interp(), photo, &imageBlock, 0, 0, width, height, 
//          compositing? TK_PHOTO_COMPOSITE_OVERLAY:TK_PHOTO_COMPOSITE_SET);
//#endif
      }
      void blit() {blit(0,0,imageBlock.width, imageBlock.height);}
      void requestRedraw() {blit();}
    };
      
    class CairoImage
    {

    protected:
      void resize(size_t width, size_t height);

      /// resize photo and surface
      /// Complaint draw() routines should set the transformation
      /// matrix to this, rather than the identity matrix, to allow
      /// scaling. identMatrix is automatically translated to the
      /// centre of the cairoSurface
      double xScale, yScale; ///< track scale requests
      double m_scale, rotation;///< track setMatrix requests

    public:
      /// configSpecs for this type (itemconfigure options)
      static Tk_ConfigSpec configSpecs[];
      
      SurfacePtr cairoSurface;
 
      /// sets the transformation matrix so that user coordinates are
      /// centered on the bitmap, and scaled according to the current
      /// scale value
      void initMatrix();

      /// implementation of -scale and -rotation item options
      void setMatrix(double scale, double rotation)
      {m_scale=scale, this->rotation=rotation;}

      // apply an additional scale transformation
      void scale(double xScale, double yScale) {
        if (xScale!=1 || yScale!=1) 
          {this->xScale*=xScale; this->yScale*=yScale;}
      }

      CairoImage():  xScale(1), yScale(1) {setMatrix(1,0);}
      CairoImage(const SurfacePtr& cairoSurface):
        xScale(1), yScale(1), cairoSurface(cairoSurface) {setMatrix(1,0);}

      virtual ~CairoImage() {}

      void attachToImage(const std::string& imgName);

      /// distance x, y is from CairoItem (in device coordinates)
      double distanceFrom(double x, double y) const;

      /// bounding box (in device coordinates) of the clip region
      virtual ecolab::array_ns::array<double> boundingBox();

      /// return -1, 0, 1 if rectangle is outside, partially inside or
      /// completely inside the clip. Arguments are in device
      /// coordinates
      int inClip(double x0, double y0, double x1, double y1) const;

      /// draw this item. @throw nothing.
      virtual void draw()=0;
      void redrawIfSurfaceTooSmall();
      void blit() {cairoSurface->blit();}

    };

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

    
    /// internal structure used for Tk Canvas cairo image items
    struct ImageItem  {
      Tk_Item header;		/* Generic stuff that's the same for all
				 * types. MUST BE FIRST IN STRUCTURE. */
      Tk_Canvas canvas;		/* Canvas containing the image. */
      double x, y;		/* Coordinates of positioning point for
				 * image. */
      double scale;             /* scale = 1 means cairo drawing
                                   coordinates in the range [-1,1] map
                                   to [x0,x1], [y0,y1] of the image
                                   pixmap */

      double rotation;          /* rotation opf cairo user coordinate
                                   with respect to pixmap coordinates,
                                   in radians */

      Tk_Anchor anchor;		/* Where to anchor image relative to (x,y). */
      char *imageString;		/* String describing -image option
                                         * (malloc-ed). NULL means no image right
				 * now. */
      char *activeImageString;	/* String describing -activeimage option.
				 * NULL means no image right now. */
      char *disabledImageString;	/* String describing -disabledimage option.
                                         * NULL means no image right now. */
      Tk_Image image;		/* Image to display in window, or NULL if no
				 * image at present. */
      Tk_Image activeImage;	/* Image to display in window, or NULL if no
				 * image at present. */
      Tk_Image disabledImage;	/* Image to display in window, or NULL if no
				 * image at present. */
      bool redrawing; // set this flag to disable enqueing of eventually redraw
      CairoImage *cairoItem;
    };

    namespace TkImageCode
    {
      int CreateImage(Tcl_Interp *interp, Tk_Canvas canvas, Tk_Item *itemPtr, 
                      int objc,Tcl_Obj *CONST objv[],Tk_ConfigSpec[]);
      void DeleteImage(Tk_Canvas canvas, Tk_Item *itemPtr, Display *display);
      /// generic configure function, given a particular \a configSpecs
      int configureCairoItem(Tcl_Interp *interp, Tk_Canvas canvas, 
                             Tk_Item *itemPtr,	int objc, 
                             Tcl_Obj *CONST objv[],
                             int flags, Tk_ConfigSpec configSpecs[]);
      void  ComputeImageBbox(Tk_Canvas canvas, ImageItem *imgPtr);
    }

    /// factory method for creating a CairoImage, to be used for
    /// creating Tk canvas itemTypes
    template <class C>
    int createImage(Tcl_Interp *interp,	Tk_Canvas canvas, Tk_Item *itemPtr, 
                    int objc,Tcl_Obj *CONST objv[])
    {
      if (TkImageCode::CreateImage(interp,canvas,itemPtr,objc,objv,C::configSpecs)==TCL_OK)
        {
          ImageItem* imgPtr=(ImageItem*)(itemPtr);
          imgPtr->cairoItem = new C;
          imgPtr->cairoItem->setMatrix(imgPtr->scale, imgPtr->rotation);
          TkImageCode::ComputeImageBbox(canvas, imgPtr);
          return TCL_OK;
        }
      TkImageCode::DeleteImage(canvas, itemPtr, 
                               Tk_Display(Tk_CanvasTkwin(canvas)));
      return TCL_ERROR;
    }

    const Tk_ItemType& cairoItemType();
#endif  // #ifdef TK

  }
}

namespace classdesc
{
  template <> struct tn<ecolab::cairo::Surface>
  {
    static string name() {return "ecolab::cairo::Surface";}
  };
}

#include "cairo_base.cd"
#endif
#endif
