/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/


#if defined(CAIRO) && defined(TK)
#include "cairo_base.h"
#include "TCL_obj_base.h"
#include "ecolab_epilogue.h"
#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo/cairo-xlib.h>
#endif
// CYGWIN has problems with WIN32_SURFACE
#if defined(CAIRO_HAS_WIN32_SURFACE) && !defined(__CYGWIN__)
#undef Realloc
#include <cairo/cairo-win32.h>
// undocumented internal function for extracting the HDC from a Drawable
extern "C" HDC TkWinGetDrawableDC(Display*, Drawable, void*);
extern "C" HDC TkWinReleaseDrawableDC(Drawable, HDC, void*);
#endif
#if defined(MAC_OSX_TK)
// MacOSX Tk code uses the deprecated Quickdraw API, and doesnt seem
// to provide any way of obtaining a CGContextRef needed to use quartz
// cairo surfaces

// on MacOSX Carbon builds, the strategy is therefore to render to a
// TkImage, then blit that.
#define TKPHOTOSURFACE
#endif
#include <memory>
using namespace std;

int ecolab_cairo_types_link;



namespace ecolab
{
  namespace cairo
  {

    namespace
    {
      // manage an anonymous Tk photo image.
      struct Image
      {
        string name;
        Image() {
          tclcmd cmd;
          cmd << "image create photo\n";
          name=cmd.result;
        }
        // protect with interp in case destructor called after interpreter exited
        ~Image() {if (!interpExiting) tclcmd() << "image delete"<<name<<"\n";}
      };

      //A version of TkPhotoSurface that manages its own Tk image
      struct TkCanvasPhotoSurface: public Image, public ecolab::cairo::TkPhotoSurface
      {
        Tk_Image image; // save the image for decoupling changedproc at right time
        TkCanvasPhotoSurface():
          TkPhotoSurface(Tk_FindPhoto(interp(), name.c_str())), image(NULL) {}
        // protect with interp in case destructor called after interpreter exited
        ~TkCanvasPhotoSurface() {if (!interpExiting) Tk_FreeImage(image);}
      };


      // A version of Surface allowing notifications of redraw
      struct CanvasSurface: public Surface, public Tk_Item
      {
        Tk_Canvas canvas;
        CanvasSurface(cairo_surface_t* s, Tk_Canvas canvas, Tk_Item header): 
          Surface(s), Tk_Item(header), canvas(canvas) {}
        void requestRedraw() {
          Tk_CanvasEventuallyRedraw(canvas, x1, y1, x2, y2);
          // force redraw to happen immediately, to prevent Tk from
          // unnecessarily redrawing other items in between the items
          // being redrawn
          tclcmd() << "update\n";
        }
      };
    }

    static Tk_CustomOption tagsOption = {
      (Tk_OptionParseProc *) Tk_CanvasTagsParseProc,
      Tk_CanvasTagsPrintProc, (ClientData) NULL
    };

#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
    Tk_ConfigSpec CairoImage::configSpecs[] =
      {
        //-image is deprecated, and not used
        {TK_CONFIG_STRING, "-image", NULL, NULL,
         NULL, Tk_Offset(ImageItem, imageString), TK_CONFIG_NULL_OK},
        {TK_CONFIG_DOUBLE, "-scale", NULL, NULL,
         "1.0", Tk_Offset(ImageItem, scale), TK_CONFIG_NULL_OK},
        {TK_CONFIG_DOUBLE, "-rotation", NULL, NULL,
         "0.0", Tk_Offset(ImageItem, rotation), TK_CONFIG_NULL_OK},
        {TK_CONFIG_CUSTOM, "-tags", NULL, NULL,
         NULL, 0, TK_CONFIG_NULL_OK, &tagsOption},
        {TK_CONFIG_END}
      };
#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic pop
#endif

    namespace TkImageCode
    {
      /*
        fugly code grabbed out of tkCanvImg.c because it works
      */

      typedef struct TagSearchExpr_s TagSearchExpr;

      struct TagSearchExpr_s {
        TagSearchExpr *next;	/* For linked lists of expressions - used in
				 * bindings. */
        Tk_Uid uid;			/* The uid of the whole expression. */
        Tk_Uid *uids;		/* Expresion compiled to an array of uids. */
        int allocated;		/* Available space for array of uids. */
        int length;			/* Length of expression. */
        int index;			/* Current position in expression
                                         * evaluation. */
        int match;			/* This expression matches event's item's
                                         * tags. */
      };
      /*
       * The record below describes a canvas widget. It is made available to the
       * item functions so they can access certain shared fields such as the overall
       * displacement and scale factor for the canvas.
       */

      typedef struct TkCanvas {
        Tk_Window tkwin;		/* Window that embodies the canvas. NULL means
                                         * that the window has been destroyed but the
                                         * data structures haven't yet been cleaned
                                         * up.*/
        Display *display;		/* Display containing widget; needed, among
                                         * other things, to release resources after
                                         * tkwin has already gone away. */
        Tcl_Interp *interp;		/* Interpreter associated with canvas. */
        Tcl_Command widgetCmd;	/* Token for canvas's widget command. */
        Tk_Item *firstItemPtr;	/* First in list of all items in canvas, or
				 * NULL if canvas empty. */
        Tk_Item *lastItemPtr;	/* Last in list of all items in canvas, or
				 * NULL if canvas empty. */

        /*
         * Information used when displaying widget:
         */

        int borderWidth;		/* Width of 3-D border around window. */
        Tk_3DBorder bgBorder;	/* Used for canvas background. */
        int relief;			/* Indicates whether window as a whole is
                                         * raised, sunken, or flat. */
        int highlightWidth;		/* Width in pixels of highlight to draw around
                                         * widget when it has the focus. <= 0 means
                                         * don't draw a highlight. */
        XColor *highlightBgColorPtr;
        /* Color for drawing traversal highlight area
         * when highlight is off. */
        XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
        int inset;			/* Total width of all borders, including
                                         * traversal highlight and 3-D border.
                                         * Indicates how much interior stuff must be
                                         * offset from outside edges to leave room for
                                         * borders. */
        GC pixmapGC;		/* Used to copy bits from a pixmap to the
				 * screen and also to clear the pixmap. */
        int width, height;		/* Dimensions to request for canvas window,
                                         * specified in pixels. */
        int redrawX1, redrawY1;	/* Upper left corner of area to redraw, in
				 * pixel coordinates. Border pixels are
				 * included. Only valid if REDRAW_PENDING flag
				 * is set. */
        int redrawX2, redrawY2;	/* Lower right corner of area to redraw, in
				 * integer canvas coordinates. Border pixels
				 * will *not* be redrawn. */
        int confine;		/* Non-zero means constrain view to keep as
				 * much of canvas visible as possible. */

        /*
         * Information used to manage the selection and insertion cursor:
         */

        Tk_CanvasTextInfo textInfo; /* Contains lots of fields; see tk.h for
                                     * details. This structure is shared with the
                                     * code that implements individual items. */
        int insertOnTime;		/* Number of milliseconds cursor should spend
                                         * in "on" state for each blink. */
        int insertOffTime;		/* Number of milliseconds cursor should spend
                                         * in "off" state for each blink. */
        Tcl_TimerToken insertBlinkHandler;
        /* Timer handler used to blink cursor on and
         * off. */

        /*
         * Transformation applied to canvas as a whole: to compute screen
         * coordinates (X,Y) from canvas coordinates (x,y), do the following:
         *
         * X = x - xOrigin;
         * Y = y - yOrigin;
         */

        int xOrigin, yOrigin;	/* Canvas coordinates corresponding to
				 * upper-left corner of window, given in
				 * canvas pixel units. */
        int drawableXOrigin, drawableYOrigin;
        /* During redisplay, these fields give the
         * canvas coordinates corresponding to the
         * upper-left corner of the drawable where
         * items are actually being drawn (typically a
         * pixmap smaller than the whole window). */

        /*
         * Information used for event bindings associated with items.
         */

        Tk_BindingTable bindingTable;
        /* Table of all bindings currently defined for
         * this canvas. NULL means that no bindings
         * exist, so the table hasn't been created.
         * Each "object" used for this table is either
         * a Tk_Uid for a tag or the address of an
         * item named by id. */
        Tk_Item *currentItemPtr;	/* The item currently containing the mouse
                                         * pointer, or NULL if none. */
        Tk_Item *newCurrentPtr;	/* The item that is about to become the
				 * current one, or NULL. This field is used to
				 * detect deletions of the new current item
				 * pointer that occur during Leave processing
				 * of the previous current item. */
        double closeEnough;		/* The mouse is assumed to be inside an item
                                         * if it is this close to it. */
        XEvent pickEvent;		/* The event upon which the current choice of
                                         * currentItem is based. Must be saved so that
                                         * if the currentItem is deleted, can pick
                                         * another. */
        int state;			/* Last known modifier state. Used to defer
                                         * picking a new current object while buttons
                                         * are down. */

        /*
         * Information used for managing scrollbars:
         */

        char *xScrollCmd;		/* Command prefix for communicating with
                                         * horizontal scrollbar. NULL means no
                                         * horizontal scrollbar. Malloc'ed. */
        char *yScrollCmd;		/* Command prefix for communicating with
                                         * vertical scrollbar. NULL means no vertical
                                         * scrollbar. Malloc'ed. */
        int scrollX1, scrollY1, scrollX2, scrollY2;
        /* These four coordinates define the region
         * that is the 100% area for scrolling (i.e.
         * these numbers determine the size and
         * location of the sliders on scrollbars).
         * Units are pixels in canvas coords. */
        char *regionString;		/* The option string from which scrollX1 etc.
                                         * are derived. Malloc'ed. */
        int xScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling. This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */
        int yScrollIncrement;	/* If >0, defines a grid for horizontal
				 * scrolling. This is the size of the "unit",
				 * and the left edge of the screen will always
				 * lie on an even unit boundary. */

        /*
         * Information used for scanning:
         */

        int scanX;			/* X-position at which scan started (e.g.
                                         * button was pressed here). */
        int scanXOrigin;		/* Value of xOrigin field when scan started. */
        int scanY;			/* Y-position at which scan started (e.g.
                                         * button was pressed here). */
        int scanYOrigin;		/* Value of yOrigin field when scan started. */

        /*
         * Information used to speed up searches by remembering the last item
         * created or found with an item id search.
         */

        Tk_Item *hotPtr;		/* Pointer to "hot" item (one that's been
                                         * recently used. NULL means there's no hot
                                         * item. */
        Tk_Item *hotPrevPtr;	/* Pointer to predecessor to hotPtr (NULL
				 * means item is first in list). This is only
				 * a hint and may not really be hotPtr's
				 * predecessor. */

        /*
         * Miscellaneous information:
         */

        Tk_Cursor cursor;		/* Current cursor for window, or None. */
        char *takeFocus;		/* Value of -takefocus option; not used in the
                                         * C code, but used by keyboard traversal
                                         * scripts. Malloc'ed, but may be NULL. */
        double pixelsPerMM;		/* Scale factor between MM and pixels; used
                                         * when converting coordinates. */
        int flags;			/* Various flags; see below for
                                         * definitions. */
        int nextId;			/* Number to use as id for next item created
                                         * in widget. */
        Tk_PostscriptInfo psInfo;	/* Pointer to information used for generating
                                         * Postscript for the canvas. NULL means no
                                         * Postscript is currently being generated. */
        Tcl_HashTable idTable;	/* Table of integer indices. */

        /*
         * Additional information, added by the 'dash'-patch
         */

        void *reserved1;
        Tk_State canvas_state;	/* State of canvas. */
        void *reserved2;
        void *reserved3;
        Tk_TSOffset tsoffset;
#ifndef USE_OLD_TAG_SEARCH
        TagSearchExpr *bindTagExprs;/* Linked list of tag expressions used in
                                     * bindings. */
#endif
      } TkCanvas;

      /*
       *--------------------------------------------------------------
       *
       * ComputeImageBbox --
       *
       *	This function is invoked to compute the bounding box of all the pixels
       *	that may be drawn as part of a image item. This function is where the
       *	child image's placement is computed.
       *
       * Results:
       *	None.
       *
       * Side effects:
       *	The fields x1, y1, x2, and y2 are updated in the header for itemPtr.
       *
       *--------------------------------------------------------------
       */

      /* ARGSUSED */
      void
      ComputeImageBbox(
                       Tk_Canvas canvas,		/* Canvas that contains item. */
                       ImageItem *imgPtr)		/* Item whose bbox is to be recomputed. */
      {

       if (imgPtr->cairoItem)
         {
           array<double> bbox(4,0);
           bbox=imgPtr->cairoItem->boundingBox();

           /*
            * Store the information in the item header.
            */

           imgPtr->header.x1 = bbox[0]+imgPtr->x;
           imgPtr->header.y1 = bbox[1]+imgPtr->y;
           imgPtr->header.x2 = bbox[2]+imgPtr->x+1;
           imgPtr->header.y2 = bbox[3]+imgPtr->y+1;
         }
      }

      /*
       *----------------------------------------------------------------------
       *
       * ImageChangedProc --
       *
       *	This function is invoked by the image code whenever the manager for an
       *	image does something that affects the image's size or how it is
       *	displayed.
       *
       * Results:
       *	None.
       *
       * Side effects:
       *	Arranges for the canvas to get redisplayed.
       *
       *----------------------------------------------------------------------
       */

#ifdef TKPHOTOSURFACE
      static void
      ImageChangedProc(
                       ClientData clientData,	/* Pointer to canvas item for image. */
                       int x, int y,		/* Upper left pixel (within image) that must
                                                 * be redisplayed. */
                       int width, int height,	/* Dimensions of area to redisplay (may be <=
                                                 * 0). */
                       int imgWidth, int imgHeight)/* New dimensions of image. */
      {
        if (interpExiting) return;
        ImageItem *imgPtr = (ImageItem *) clientData;
        if (!imgPtr->redrawing)
          {

            /*
             * If the image's size changed and it's not anchored at its northwest
             * corner then just redisplay the entire area of the image. This is a bit
             * over-conservative, but we need to do something because a size change
             * also means a position change.
             */

            if (((imgPtr->header.x2 - imgPtr->header.x1) != imgWidth)
                || ((imgPtr->header.y2 - imgPtr->header.y1) != imgHeight)) {
              x = y = 0;
              width = imgWidth;
              height = imgHeight;
              Tk_CanvasEventuallyRedraw(imgPtr->canvas, imgPtr->header.x1,
                                        imgPtr->header.y1, imgPtr->header.x2, imgPtr->header.y2);
            }
            ComputeImageBbox(imgPtr->canvas, imgPtr);
            Tk_CanvasEventuallyRedraw(imgPtr->canvas, imgPtr->header.x1 + x,
                                      imgPtr->header.y1 + y, (int) (imgPtr->header.x1 + x + width),
                                      (int) (imgPtr->header.y1 + y + height));
          }
      }
#endif

      /*
       *--------------------------------------------------------------
       *
       * ImageCoords --
       *
       *	This function is invoked to process the "coords" widget command on
       *	image items. See the user documentation for details on what it does.
       *
       * Results:
       *	Returns TCL_OK or TCL_ERROR, and sets the interp's result.
       *
       * Side effects:
       *	The coordinates for the given item may be changed.
       *
       *--------------------------------------------------------------
       */

      static int
      ImageCoords(
                  Tcl_Interp *interp,		/* Used for error reporting. */
                  Tk_Canvas canvas,		/* Canvas containing item. */
                  Tk_Item *itemPtr,		/* Item whose coordinates are to be read or
                                                 * modified. */
                  int objc,			/* Number of coordinates supplied in objv. */
                  Tcl_Obj *CONST objv[])	/* Array of coordinates: x1, y1, x2, y2, ... */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;

        if (objc == 0) {
          Tcl_Obj *obj = Tcl_NewObj();

          Tcl_Obj *subobj = Tcl_NewDoubleObj(imgPtr->x);
          Tcl_ListObjAppendElement(interp, obj, subobj);
          subobj = Tcl_NewDoubleObj(imgPtr->y);
          Tcl_ListObjAppendElement(interp, obj, subobj);
          Tcl_SetObjResult(interp, obj);
        } else if (objc < 3) {
          if (objc==1) {
	    if (Tcl_ListObjGetElements(interp, objv[0], &objc,
                                       (Tcl_Obj ***) &objv) != TCL_OK) {
              return TCL_ERROR;
	    } else if (objc != 2) {
              char buf[64];

              sprintf(buf, "wrong # coordinates: expected 2, got %d", objc);
              Tcl_SetResult(interp, buf, TCL_VOLATILE);
              return TCL_ERROR;
	    }
          }
          if ((Tk_CanvasGetCoordFromObj(interp, canvas, objv[0], &imgPtr->x) != TCL_OK)
              || (Tk_CanvasGetCoordFromObj(interp, canvas, objv[1],
                                           &imgPtr->y) != TCL_OK)) {
	    return TCL_ERROR;
          }
          ComputeImageBbox(canvas, imgPtr);
        } else {
          char buf[64];

          sprintf(buf, "wrong # coordinates: expected 0 or 2, got %d", objc);
          Tcl_SetResult(interp, buf, TCL_VOLATILE);
          return TCL_ERROR;
        }
        return TCL_OK;
      }

      /*
       *--------------------------------------------------------------
       *
       * ConfigureImage --
       *
       *	This function is invoked to configure various aspects of an image
       *	item, such as its anchor position.
       *
       * Results:
       *	A standard Tcl result code. If an error occurs, then an error message
       *	is left in the interp's result.
       *
       * Side effects:
       *	Configuration information may be set for itemPtr.
       *
       *--------------------------------------------------------------
       */

      int
      configureCairoItem(
                     Tcl_Interp *interp,		/* Used for error reporting. */
                     Tk_Canvas canvas,		/* Canvas containing itemPtr. */
                     Tk_Item *itemPtr,		/* Image item to reconfigure. */
                     int objc,			/* Number of elements in objv.  */
                     Tcl_Obj *CONST objv[],	/* Arguments describing things to configure. */
                     int flags, Tk_ConfigSpec configSpecs[])			/* Flags to pass to Tk_ConfigureWidget. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;
        Tk_Window tkwin;

        tkwin = Tk_CanvasTkwin(canvas);
        if (TCL_OK != Tk_ConfigureWidget(interp, tkwin, configSpecs, objc,
                                         (CONST char **) objv, (char *) imgPtr, flags|TK_CONFIG_OBJS)) {
          return TCL_ERROR;
        }

        /*
         * Create the image. Save the old image around and don't free it until
         * after the new one is allocated. This keeps the reference count from
         * going to zero so the image doesn't have to be recreated if it hasn't
         * changed.
         */

//        if (imgPtr->activeImageString != NULL) {
//          itemPtr->redraw_flags |= TK_ITEM_STATE_DEPENDANT;
//        } else {
//          itemPtr->redraw_flags &= ~TK_ITEM_STATE_DEPENDANT;
          //        }
//        if (imgPtr->imageString != NULL) {
//          image = Tk_GetImage(interp, tkwin, imgPtr->imageString,
//                              ImageChangedProc, (ClientData) imgPtr);
//          if (image == NULL) {
//	    return TCL_ERROR;
//          }
//        } else {
//          image = NULL;
//        }
//        if (imgPtr->image != NULL) {
//          Tk_FreeImage(imgPtr->image);
//        }
//        imgPtr->image = image;
//        if (imgPtr->activeImageString != NULL) {
//          image = Tk_GetImage(interp, tkwin, imgPtr->activeImageString,
//                              ImageChangedProc, (ClientData) imgPtr);
//          if (image == NULL) {
//	    return TCL_ERROR;
//          }
//        } else {
//          image = NULL;
//        }
//        if (imgPtr->activeImage != NULL) {
//          Tk_FreeImage(imgPtr->activeImage);
//        }
//        imgPtr->activeImage = image;
//        if (imgPtr->disabledImageString != NULL) {
//          image = Tk_GetImage(interp, tkwin, imgPtr->disabledImageString,
//                              ImageChangedProc, (ClientData) imgPtr);
//          if (image == NULL) {
//	    return TCL_ERROR;
//          }
//        } else {
//          image = NULL;
//        }
//        if (imgPtr->disabledImage != NULL) {
//          Tk_FreeImage(imgPtr->disabledImage);
//        }
//        imgPtr->disabledImage = image;
        ComputeImageBbox(canvas, imgPtr);

        if (imgPtr->cairoItem)
          imgPtr->cairoItem->setMatrix(imgPtr->scale, imgPtr->rotation);
        return TCL_OK;
      }

      static int ConfigureImage(
                     Tcl_Interp *interp,		/* Used for error reporting. */
                     Tk_Canvas canvas,		/* Canvas containing itemPtr. */
                     Tk_Item *itemPtr,		/* Image item to reconfigure. */
                     int objc,			/* Number of elements in objv.  */
                     Tcl_Obj *CONST objv[],	/* Arguments describing things to configure. */
                     int flags)
      {
        return configureCairoItem
          (interp, canvas, itemPtr,objc, objv, flags, CairoImage::configSpecs);	
      }

      /*
       *--------------------------------------------------------------
       *
       * DeleteImage --
       *
       *	This function is called to clean up the data structure associated with
       *	a image item.
       *
       * Results:
       *	None.
       *
       * Side effects:
       *	Resources associated with itemPtr are released.
       *
       *--------------------------------------------------------------
       */

      void
      DeleteImage(
                  Tk_Canvas canvas,		/* Info about overall canvas widget. */
                  Tk_Item *itemPtr,		/* Item that is being deleted. */
                  Display *display)		/* Display containing window for canvas. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;
//        // do a free image here, to prevent FreeImage being called
//        // after Tk has shut down if a referance to the surface is
//        // being held elsewhere
//        if (imgPtr->cairoItem)
//          if (TkCanvasPhotoSurface* csurf=dynamic_cast<TkCanvasPhotoSurface*>
//              (imgPtr->cairoItem->cairoSurface.get()))
//            {
//              Tk_FreeImage(csurf->image);
//              csurf->image=NULL;
//            }
//
        delete imgPtr->cairoItem;
        imgPtr->cairoItem=NULL;

        if (imgPtr->imageString != NULL) {
          ckfree(imgPtr->imageString);
        }
//        if (imgPtr->activeImageString != NULL) {
//          ckfree(imgPtr->activeImageString);
//        }
//        if (imgPtr->disabledImageString != NULL) {
//          ckfree(imgPtr->disabledImageString);
//        }
//        if (imgPtr->image != NULL) {
//          Tk_FreeImage(imgPtr->image);
//        }
//        if (imgPtr->activeImage != NULL) {
//          Tk_FreeImage(imgPtr->activeImage);
//        }
//        if (imgPtr->disabledImage != NULL) {
//          Tk_FreeImage(imgPtr->disabledImage);
//        }
      }

      /*
       *--------------------------------------------------------------
       *
       * CreateImage --
       *
       *	This function is invoked to create a new image item in a canvas.
       *
       * Results:
       *	A standard Tcl return value. If an error occurred in creating the
       *	item, then an error message is left in the interp's result; in this
       *	case itemPtr is left uninitialized, so it can be safely freed by the
       *	caller.
       *
       * Side effects:
       *	A new image item is created.
       *
       *--------------------------------------------------------------
       */

      int
      CreateImage(
                  Tcl_Interp *interp,		/* Interpreter for error reporting. */
                  Tk_Canvas canvas,		/* Canvas to hold new item. */
                  Tk_Item *itemPtr,		/* Record to hold new item; header has been
                                                 * initialized by caller. */
                  int objc,			/* Number of arguments in objv. */
                  Tcl_Obj *CONST objv[],Tk_ConfigSpec configSpecs[])	/* Arguments describing rectangle. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;
        int i;

        if (objc == 0) {
          Tcl_Panic("canvas did not pass any coords\n");
        }

        /*
         * Initialize item's record.
         */

        imgPtr->canvas = canvas;
        imgPtr->anchor = TK_ANCHOR_CENTER;
        imgPtr->imageString = NULL;
        imgPtr->activeImageString = NULL;
        imgPtr->disabledImageString = NULL;
        imgPtr->image = NULL;
        imgPtr->activeImage = NULL;
        imgPtr->disabledImage = NULL;
        imgPtr->redrawing = false;
        imgPtr->cairoItem = NULL;

        /*
         * Process the arguments to fill in the item record. Only 1 (list) or 2 (x
         * y) coords are allowed.
         */

        if (objc == 1) {
          i = 1;
        } else {
          char *arg = Tcl_GetString(objv[1]);
          i = 2;
          if ((arg[0] == '-') && (arg[1] >= 'a') && (arg[1] <= 'z')) {
	    i = 1;
          }
        }

        if (configureCairoItem
            (interp, canvas, itemPtr, objc-i, objv+i, 0, configSpecs) == TCL_OK
            && ImageCoords(interp, canvas, itemPtr, i, objv) == TCL_OK)
          return TCL_OK;

        return TCL_ERROR;
      }


      /*
       *--------------------------------------------------------------
       *
       * DisplayImage --
       *
       *	This function is invoked to draw a image item in a given drawable.
       *
       * Results:
       *	None.
       *
       * Side effects:
       *	ItemPtr is drawn in drawable using the transformation information in
       *	canvas.
       *
       *--------------------------------------------------------------
       */

      static void
      DisplayImage(
                   Tk_Canvas canvas,		/* Canvas that contains item. */
                   Tk_Item *itemPtr,		/* Item to be displayed. */
                   Display *display,		/* Display on which to draw item. */
                   Drawable drawable,		/* Pixmap or window in which to draw item. */
                   int x, int y, int width, int height)
      /* Describes region of canvas that must be
       * redisplayed (not used). */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;
        Tk_Window win=Tk_CanvasTkwin(canvas);
        short drawableX, drawableY;
#ifdef TKPHOTOSURFACE
        if (!imgPtr->cairoItem->cairoSurface)
          {
            TkCanvasPhotoSurface* tkCanvasPS=new TkCanvasPhotoSurface;
            tkCanvasPS->image = Tk_GetImage
              (interp(), win, tkCanvasPS->name.c_str(), 
               ImageChangedProc, imgPtr);

            imgPtr->cairoItem->cairoSurface.reset(tkCanvasPS);
          }

        Surface& surf=*imgPtr->cairoItem->cairoSurface;
        int w=imgPtr->header.x2-imgPtr->header.x1,
          h=imgPtr->header.y2-imgPtr->header.y1;
        int x1=imgPtr->header.x1, y1=imgPtr->header.y1;
        imgPtr->redrawing=true; // disable extra redraws due to ImageChanged proc being called
        surf.resize(w,h);
        surf.clear();
        cairo_surface_set_device_offset(surf.surface(), imgPtr->x-x1, imgPtr->y-y1);
        try {imgPtr->cairoItem->draw();}
        catch (const std::exception& e) 
          {cerr<<"illegal exception caught in draw()"<<e.what()<<endl;}
        catch (...) {cerr<<"illegal exception caught in draw()";}
        surf.blit();
        Tk_CanvasDrawableCoords(canvas, x, y, &drawableX, &drawableY);
        if (TkCanvasPhotoSurface* csurf=dynamic_cast<TkCanvasPhotoSurface*>
            (imgPtr->cairoItem->cairoSurface.get()))
            Tk_RedrawImage(csurf->image, x-x1, y-y1, width, height, 
                       drawable, drawableX, drawableY);
        imgPtr->redrawing=false;
        return;

#elif defined(CAIRO_HAS_WIN32_SURFACE)  && !defined(__CYGWIN__)
        // TkWinGetDrawableDC is an internal (ie undocumented) routine
        // for getting the DC. We need to declare something to take
        // the state parameter - two long longs should be ample here
        long long state[2];
        HDC hdc=TkWinGetDrawableDC(display, drawable, state);
        SaveDC(hdc);
        imgPtr->cairoItem->cairoSurface.reset
          (new CanvasSurface(cairo_win32_surface_create(hdc),
                             canvas, imgPtr->header));
#elif defined(MAC_OSX_TK)
        // Could not find *any way* of obtaining a CGContextRef needed to use a Cairo Quartz surface!
#else // X11 code
        int depth;
        Visual *visual = Tk_GetVisual(interp(), win, "default", &depth, NULL);
        imgPtr->cairoItem->cairoSurface.reset
          (new CanvasSurface
           (cairo_xlib_surface_create
            (display, drawable, visual, Tk_Width(win), Tk_Height(win)),
            canvas, imgPtr->header));
#endif

        Tk_CanvasDrawableCoords(canvas, imgPtr->x, imgPtr->y,
                                &drawableX, &drawableY);
        cairo_surface_set_device_offset
          (imgPtr->cairoItem->cairoSurface->surface(), drawableX, drawableY);

        try {imgPtr->cairoItem->draw();}
        catch (const std::exception& e) 
          {cerr<<"illegal exception caught in draw()"<<e.what()<<endl;}
        catch (...) {cerr<<"illegal exception caught in draw()";}
//        cairo_surface_flush(imgPtr->cairoItem->cairoSurface->surface());
//        cairo_surface_finish(imgPtr->cairoItem->cairoSurface->surface());
        imgPtr->cairoItem->cairoSurface->surface(NULL);
#if defined(CAIRO_HAS_WIN32_SURFACE) && !defined(__CYGWIN__) && !defined(TKPHOTOSURFACE)
        RestoreDC(hdc,-1);
        TkWinReleaseDrawableDC(drawable, hdc, state);
#endif
      }

      /*
       *--------------------------------------------------------------
       *
       * ImageToPoint --
       *
       *	Computes the distance from a given point to a given rectangle, in
       *	canvas units.
       *
       * Results:
       *	The return value is 0 if the point whose x and y coordinates are
       *	coordPtr[0] and coordPtr[1] is inside the image. If the point isn't
       *	inside the image then the return value is the distance from the point
       *	to the image.
       *
       * Side effects:
       *	None.
       *
       *--------------------------------------------------------------
       */

      static double
      ImageToPoint(
                   Tk_Canvas canvas,		/* Canvas containing item. */
                   Tk_Item *itemPtr,		/* Item to check against point. */
                   double *coordPtr)		/* Pointer to x and y coordinates. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;
        double x1, x2, y1, y2, xDiff, yDiff;
        x1 = imgPtr->header.x1;
        y1 = imgPtr->header.y1;
        x2 = imgPtr->header.x2;
        y2 = imgPtr->header.y2;
        /*
         * Point is outside rectangle.
         */
        if (coordPtr[0] < x1) {
          xDiff = x1 - coordPtr[0];
        } else if (coordPtr[0] > x2) {
          xDiff = coordPtr[0] - x2;
        } else {
          xDiff = 0;
        }
        if (coordPtr[1] < y1) {
          yDiff = y1 - coordPtr[1];
        } else if (coordPtr[1] > y2) {
          yDiff = coordPtr[1] - y2;
        } else {
          yDiff = 0;
        }
        return hypot(xDiff, yDiff);
      }

      /*
       *--------------------------------------------------------------
       *
       * ImageToArea --
       *
       *	This function is called to determine whether an item lies entirely
       *	inside, entirely outside, or overlapping a given rectangle.
       *
       * Results:
       *	-1 is returned if the item is entirely outside the area given by
       *	rectPtr, 0 if it overlaps, and 1 if it is entirely inside the given
       *	area.
       *
       * Side effects:
       *	None.
       *
       *--------------------------------------------------------------
       */

      static int
      ImageToArea(
                  Tk_Canvas canvas,		/* Canvas containing item. */
                  Tk_Item *itemPtr,		/* Item to check against rectangle. */
                  double *rectPtr)		/* Pointer to array of four coordinates
                                                 * (x1,y1,x2,y2) describing rectangular
                                                 * area. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;
        if ((rectPtr[2] <= imgPtr->header.x1)
            || (rectPtr[0] >= imgPtr->header.x2)
            || (rectPtr[3] <= imgPtr->header.y1)
            || (rectPtr[1] >= imgPtr->header.y2)) {
          return -1;
        }
        if ((rectPtr[0] <= imgPtr->header.x1)
            && (rectPtr[1] <= imgPtr->header.y1)
            && (rectPtr[2] >= imgPtr->header.x2)
            && (rectPtr[3] >= imgPtr->header.y2)) {
          return 1;
        }
        return 0;
      }

      /*
       *--------------------------------------------------------------
       *
       * ImageToPostscript --
       *
       *	This function is called to generate Postscript for image items.
       *
       * Results:
       *	The return value is a standard Tcl result. If an error occurs in
       *	generating Postscript then an error message is left in interp->result,
       *	replacing whatever used to be there. If no error occurs, then
       *	Postscript for the item is appended to the result.
       *
       * Side effects:
       *	None.
       *
       *--------------------------------------------------------------
       */

      static int
      ImageToPostscript(
                        Tcl_Interp *interp,		/* Leave Postscript or error message here. */
                        Tk_Canvas canvas,		/* Information about overall canvas. */
                        Tk_Item *itemPtr,		/* Item for which Postscript is wanted. */
                        int prepass)		/* 1 means this is a prepass to collect font
                                                 * information; 0 means final Postscript is
                                                 * being created.*/
      {
        return TCL_OK;
        // TODO
//        ImageItem *imgPtr = (ImageItem *)itemPtr;
//        Tk_Window canvasWin = Tk_CanvasTkwin(canvas);
//
//        char buffer[256];
//        double x, y;
//        int width, height;
//        Tk_Image image;
//        Tk_State state = itemPtr->state;
//
//        if(state == TK_STATE_NULL) {
//          state = ((TkCanvas *)canvas)->canvas_state;
//        }
//
//        image = imgPtr->image;
//        if (((TkCanvas *)canvas)->currentItemPtr == itemPtr) {
//          if (imgPtr->activeImage != NULL) {
//	    image = imgPtr->activeImage;
//          }
//        } else if (state == TK_STATE_DISABLED) {
//          if (imgPtr->disabledImage != NULL) {
//	    image = imgPtr->disabledImage;
//          }
//        }
//        if (image == NULL) {
//          /*
//           * Image item without actual image specified.
//           */
//
//          return TCL_OK;
//        }
//        Tk_SizeOfImage(image, &width, &height);
//
//        /*
//         * Compute the coordinates of the lower-left corner of the image, taking
//         * into account the anchor position for the image.
//         */
//
//        x = imgPtr->x;
//        y = Tk_CanvasPsY(canvas, imgPtr->y);
//
//        switch (imgPtr->anchor) {
//        case TK_ANCHOR_NW:			   y -= height;		break;
//        case TK_ANCHOR_N:	   x -= width/2.0; y -= height;		break;
//        case TK_ANCHOR_NE:	   x -= width;	   y -= height;		break;
//        case TK_ANCHOR_E:	   x -= width;	   y -= height/2.0;	break;
//        case TK_ANCHOR_SE:	   x -= width;				break;
//        case TK_ANCHOR_S:	   x -= width/2.0;			break;
//        case TK_ANCHOR_SW:						break;
//        case TK_ANCHOR_W:			   y -= height/2.0;	break;
//        case TK_ANCHOR_CENTER: x -= width/2.0; y -= height/2.0;	break;
//        }
//
//        if (!prepass) {
//          sprintf(buffer, "%.15g %.15g", x, y);
//          Tcl_AppendResult(interp, buffer, " translate\n", NULL);
//        }
//
//        return Tk_PostscriptImage(image, interp, canvasWin,
//                                  ((TkCanvas *) canvas)->psInfo, 0, 0, width, height, prepass);
      }

      /*
       *--------------------------------------------------------------
       *
       * ScaleImage --
       *
       *	This function is invoked to rescale an item.
       *
       * Results:
       *	None.
       *
       * Side effects:
       *	The item referred to by itemPtr is rescaled so that the following
       *	transformation is applied to all point coordinates:
       *		x' = originX + scaleX*(x-originX)
       *		y' = originY + scaleY*(y-originY)
       *
       *--------------------------------------------------------------
       */

      static void
      ScaleImage(
                 Tk_Canvas canvas,		/* Canvas containing rectangle. */
                 Tk_Item *itemPtr,		/* Rectangle to be scaled. */
                 double originX, double originY,
                 /* Origin about which to scale rect. */
                 double scaleX,		/* Amount to scale in X direction. */
                 double scaleY)		/* Amount to scale in Y direction. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;

        imgPtr->x = originX + scaleX*(imgPtr->x - originX);
        imgPtr->y = originY + scaleY*(imgPtr->y - originY);
        ComputeImageBbox(canvas, imgPtr);
        if (imgPtr->cairoItem) //imgPtr->cairoItem->draw();
          imgPtr->cairoItem->scale(scaleX, scaleY);
      }

      /*
       *--------------------------------------------------------------
       *
       * TranslateImage --
       *
       *	This function is called to move an item by a given amount.
       *
       * Results:
       *	None.
       *
       * Side effects:
       *	The position of the item is offset by (xDelta, yDelta), and the
       *	bounding box is updated in the generic part of the item structure.
       *
       *--------------------------------------------------------------
       */

      static void
      TranslateImage(
                     Tk_Canvas canvas,		/* Canvas containing item. */
                     Tk_Item *itemPtr,		/* Item that is being moved. */
                     double deltaX, double deltaY)
      /* Amount by which item is to be moved. */
      {
        ImageItem *imgPtr = (ImageItem *) itemPtr;

        imgPtr->x += deltaX;
        imgPtr->y += deltaY;
        imgPtr->header.x1+=deltaX;
        imgPtr->header.x2+=deltaX;
        imgPtr->header.y1+=deltaY;
        imgPtr->header.y2+=deltaY;
        ComputeImageBbox(canvas, imgPtr);
      }

    }


    
#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
    const Tk_ItemType& cairoItemType()
    {
      static Tk_ItemType c=
        {
          "",			/* name */
          sizeof(ImageItem),		/* itemSize */
          NULL,		/* createProc */
          CairoImage::configSpecs,		/* configSpecs */
          TkImageCode::ConfigureImage,		/* configureProc */
          TkImageCode::ImageCoords,		/* coordProc */
          TkImageCode::DeleteImage,		/* deleteProc */
          TkImageCode::DisplayImage,		/* displayProc */
          TK_CONFIG_OBJS,		/* flags + alwaysRedraw */
          TkImageCode::ImageToPoint,		/* pointProc */
          TkImageCode::ImageToArea,		/* areaProc */
          TkImageCode::ImageToPostscript,		/* postscriptProc */
          TkImageCode::ScaleImage,			/* scaleProc */
          TkImageCode::TranslateImage,		/* translateProc */
          NULL,			/* indexProc */
          NULL,			/* icursorProc */
          NULL,			/* selectionProc */
          NULL,			/* insertProc */
          NULL,			/* dTextProc */
          NULL,			/* nextPtr */
          NULL, 0, NULL, NULL     /* reserved */
        };
      // ensure Tk_Init is called.
      if (!Tk_MainWindow(interp())) Tk_Init(interp());
      return c;
    }
#if defined(__GNUC__) && !defined(__ICC)
#pragma GCC diagnostic pop
#endif
  }
}

#endif
