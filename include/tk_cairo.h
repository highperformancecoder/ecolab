/*
  @copyright Russell Standish 2025
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/
#if defined(TK) && !defined(ECOLAB_TK_CAIRO_H)
#define ECOLAB_TK_CAIRO_H
#include <tk.h>

namespace ecolab::cairo
{
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

    double width() const override {return imageBlock.width;}
    double height() const override {return imageBlock.height;}
    void clear() override {
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

    void resize(size_t width, size_t height) override
    {
      //#if TK_MAJOR_VERSION == 8 && TK_MINOR_VERSION < 5
      //        Tk_PhotoSetSize(photo, width, height);
      //#else
      //        Tk_PhotoSetSize(interp(), photo, width, height);
      //#endif
      //        init(imageBlock.transparency);
    }

    /// copy the cairo output into the TkPhoto
    void blit(unsigned x, unsigned y, unsigned width, unsigned height) override
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
    void blit() override {blit(0,0,imageBlock.width, imageBlock.height);}
    void requestRedraw() override {blit();}
  };
      
  class CairoImage
  {
    CLASSDESC_ACCESS(CairoImage);
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
                           Tcl_Obj *const objv[],
                           int flags, Tk_ConfigSpec configSpecs[]);
    void  ComputeImageBbox(Tk_Canvas canvas, ImageItem *imgPtr);
  }

  /// factory method for creating a CairoImage, to be used for
  /// creating Tk canvas itemTypes
  template <class C>
  int createImage(Tcl_Interp *interp,	Tk_Canvas canvas, Tk_Item *itemPtr, 
                  int objc,Tcl_Obj *CONST84 objv[])
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
}

#endif
