/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#ifndef PLOT_H
#define PLOT_H

#include "classdesc.h"
#include "classdesc_access.h"
#include "TCL_obj_base.h"
#include "arrays.h"

#include <string>
#include <vector>
#include <cairo_base.h>

namespace ecolab
{
  namespace cairo
  {
    struct Colour
    {
      double r, g, b, a;
    };

  }

  extern cairo::Colour palette[];
  extern const int paletteSz;

  class Plot
  {
  public:
    // transform y coordinates (handles RHS being a different scale, as
    // well as manual scaling for minsky ticket #693)
    struct XFY
    {
      bool logy;
      double scale, o, o1;
      XFY():logy(false), scale(1), o(0), o1(0)  {}
      XFY(bool logy, double scale, double o, double o1):
        logy(logy), scale(scale), o(logy? (o>0? log10(o): 0): o), o1(o1) {
      }
      double operator()(double y) const {
        return scale*((logy? log10(y): y)-o)+o1;
      }
    };
    /// boundingBox is used explictly specify legend sizing via a bounding box relative to plot size
    enum Side {left, right, boundingBox, marker};
    enum PlotType {line, bar, scatter, line_scatter};

    struct LineStyle
    {
      enum DashStyle {solid, dash, dot, dashDot};
      cairo::Colour colour{0,0,0,1};
      double width;
      double barWidth=1.5; // bar width as a fraction of available space
      DashStyle dashStyle;
      LineStyle(): width(1), dashStyle(solid) {}
      std::vector<double> dashPattern() const;
    };
      
    std::vector<LineStyle> palette;
    void extendPalette() {palette.push_back(LineStyle());}

  private:    
    string m_image;
    std::vector<std::vector<double> > x;
    std::vector<std::vector<double> > y;
    cairo::SurfacePtr surface;

    // record error message from setMinMax
    const char* msg;
    

    CLASSDESC_ACCESS(Plot);

    // internal common routine for add()
    void addNew(cairo::Surface& surf, bool doRedraw, 
                const unsigned *startPen, const unsigned *finishPen, int numPt);

    double iflogx(double x) const {return logx? log10(x): x;}
    double iflogy(double y) const {return logy? log10(y): y;}

    /// draw grid and subgrid lines from \a tick to \a tick +
    /// increment. \a vertical indicates if the grid lines are
    /// vertical or horizontal
    void drawGrid(cairo_t* cairo, double tick, double increment, bool vertical, const XFY&) const;

    void drawLegend(cairo_t*, double width, double height) const;

    void labelAxes(cairo_t*, double width, double height) const;

    bool inBounds(float x, float y, Side side) const {
      return std::isfinite(x) && std::isfinite(y) && x>=minx && x<=maxx && (!logy || y>0) &&
        (((side==left||side==marker) && y>=miny && y<=maxy)
         || (side==right && y>=miny1 && y<=maxy1));
    }

    bool displayRHSscale() const {return !penSide.empty() && miny1<maxy1;}
    bool displayLHSscale() const {return miny<maxy;}
    string axisLabel(double x, double scale, bool percent=false) const;
    bool onlyMarkers() const; ///< return true if the only data is marker data
  public:
    Plot(): palette(paletteSz), msg(NULL), nxTicks(30), nyTicks(30), fontScale(1),
            offx(0), offy(0), logx(false), logy(false), 
            grid(false), subgrid(false), 
            leadingMarker(false), autoscale(true), percent(false), 
            legend(false), legendSide(right), legendLeft(0.9), legendTop(0.95), legendFontSz(0.03), legendOffset(0.06),
            plotType(line), xtickAngle(-45), exp_threshold(4), minx(-1), maxx(1), miny(-1), maxy(1), miny1(1), maxy1(-1)
    {
      for (int i=0; i<paletteSz; ++i) palette[i].colour=ecolab::palette[i];
    }
    virtual ~Plot() {}

    unsigned nxTicks, nyTicks; ///< number of x/y-axis ticks
    double fontScale; ///< scale tick labels
    double offx, offy; ///< origin of plot within image
    bool logx, logy; ///< logarithmic plots (x/y axes)
    bool grid, subgrid; ///< draw grid options
    bool leadingMarker; ///< draw a leading marker on the curve (full draw only)
    bool autoscale; ///< autoscale plot to data
    bool percent; ///< scales y axis label by 100
    bool legend;  ///< add a legend to the plot
    Side legendSide; ///< legend drawn towards the left or right
    double legendLeft; ///< x coordinate of legend in scale 0-1 if legendSide==boundingBox
    double legendTop; ///< y coordinate of legend in scale 0-1 if legendSide==boundingBox
    double legendFontSz; ///< y coordinate of legend in scale 0-1 if legendSide==boundingBox
    const double legendOffset; ///< offset of legend text within legend box as fraction of width
    PlotType plotType;
    /// axis labels
    string xlabel, ylabel, y1label;
    double xtickAngle; ///< angle (in degrees) at which xtick labels are drawn
    /// if |log_10 (x)| < exp_threshold, do not rescale tick value
    unsigned exp_threshold;
    unsigned symbolEvery=1; ///< every symbolEvery data points, a symbol is plotted.

    /// height (or width) of an axis label in pixels
    double labelheight() const {return lh(width(), height());}
    double lh(double width, double height) const;

    cairo_surface_t* cairoSurface() const;
    int width() const;
    int height() const;
    /// plot bounding box. y1 refers to the right hand scale
    double minx, maxx, miny, maxy, miny1, maxy1;

    /// set min/max to autoscale on contained data
    void setMinMax();

    /// set/get the Tk image that this class writes to.
    string Image() const {return m_image;}
    string Image(const string& im, bool transparency=true);
    string image(TCL_args args) {
      if (args.count>0)
        {
          string imgName=args.str();
          bool transparency=true;
          if (args.count) transparency=args;
          return Image(imgName,transparency);
        }
      else
        return Image();
    }
    
    /// redraw the plot
    bool redraw();
    /// draw the plot onto a given surface
    virtual void draw(cairo::Surface&);
    void draw(cairo_t*, double width, double height) const;
    /// clear all datapoints (and redraw)
    void clear();

    /// add a datapoint
    void add(unsigned pen, double x, double y) 
    {if (surface) add(*surface,pen,x,y);}
    /// add multiple points (C=container - eg vector/array)
    template <class C> 
    void add(unsigned pen, const C& x, const C& y) 
    {if (surface) add(*surface,pen,x,y);}

    /// label a pen (for display in a legend). The string may contain
    /// pango markup.
    void labelPen(unsigned pen, const string& label);
    /// label a pen with an arbitrary graphic, given by \a label.
    void LabelPen(unsigned pen, const cairo::SurfacePtr& label);

    /// assign a side to display the scale for pen
    void assignSide(unsigned pen, Side);

    // deprecated
    void AssignSide(unsigned pen, Side side) {assignSide(pen,side);}
    //    void labelPen(TCL_args args) {LabelPen(int(args[0]), (char*)args[1]);}

    /// remove all pen labels and side associations
    void clearPenAttributes() {
      penLabel.clear(); penTextLabel.clear(); penSide.clear();
    }

    /// TCL interface
    void plot(TCL_args args); 

    //serialisation support (surface is not auto-serialisable)
    void pack(classdesc::pack_t& p) const {p<<m_image<<x<<y<<minx<<maxx<<miny<<maxy;}

    void unpack(classdesc::pack_t& p) {
        p>>m_image>>x>>y>>minx>>maxx>>miny>>maxy;
        Image(m_image);
    }

    /// add a point to the graph withour redrwaing anything
    void addPt(unsigned pen, double x, double y);

    /// export plotting data as a CSV file. @throw if an I/O error occurs
    void exportAsCSV(const std::string& filename, const string& separator) const;
    void exportAsCSV(const std::string& filename) const
    {exportAsCSV(filename,",");}

  protected: // only protected because of TCL_obj problems
    /// default formatter suitable for plots of numeric data
    static std::string defaultFormatter(double,double);
    using Formatter=std::function<std::string(double,double)>;
    /// if \a (x,y) within ([0,1],[0,1]), then paint a value box corresponding to closest curve
    /// @param tolerance - how close in user relative coordinates the mouse needs to be to a data point
    /// @param formatter - produce text label given (x,y) values 
    /// @return true if the value label changes from previous, indicating that the plot needs repainting
    bool mouseMove(double x, double y, double tolerance, Plot::Formatter formatter=defaultFormatter);
    
    /// calculates the bounding box of the legend, given current font size settings, and the height of the plot
    void legendSize(double& xoff, double& yoff, double& width, double& height, double plotWidth, double plotHeight) const;

    std::vector<std::pair<double,std::string> > xticks;
     
    void add(cairo::Surface&, unsigned pen, double x, double y);
    void add(cairo::Surface& surf, 
                   const array_ns::array<unsigned>& pens, 
                   double x1, const array_ns::array<double>& y1);

    /// add multiple pens worth of data in one hit
    void add(const array_ns::array<unsigned>& pens, double x, 
             const array_ns::array<double>& y) 
    {if (surface) add(*surface,pens,x,y);}     
    
    template <class C>
    void add(cairo::Surface& surf, unsigned pen, const C& x1, const C& y1)
    {
      assert(x1.size()==y1.size());
      bool doRedraw=false;
      if (pen>=x.size())
        {
          x.resize(pen+1);
          y.resize(pen+1);
        }
      for (size_t i=0; i<x1.size(); ++i)
        {
          doRedraw|=x1[i]<minx||x1[i]>maxx || y1[i]<miny||y1[i]>maxy;
          x[pen].push_back(x1[i]);
          y[pen].push_back(y1[i]);
        }
      addNew(surf,doRedraw,&pen,&pen+1,x1.size());
    }

    /// assign a complete curve for \a pen
    /// \a x and \a y should have the same size, if not, the larger is truncated
    void setPen(unsigned pen, const double* xx, const double* yy, size_t sz)
    {
      if (pen>=x.size())
        {
          x.resize(pen+1);
          y.resize(pen+1);
        }
      x[pen].assign(xx, xx+sz);
      y[pen].assign(yy, yy+sz);
    }

    double mouseX=-1, mouseY=-1; ///< position in user coordinates of value box
    unsigned mousePen=0; // discovered pen 
    std::string valueString;
   
  private:
    std::vector<cairo::SurfacePtr> penLabel;
    std::vector<string> penTextLabel;
    // if this is empty, all pens are on the left, and no RHS scale is drawn
    std::vector<Side> penSide;
  };
}

namespace classdesc_access
{
  template <> struct access_pack<ecolab::Plot>
  {
    template <class U>
    void operator()(classdesc::pack_t& p, const classdesc::string& d, U& a)
    {a.pack(p);}
  };

  template <> struct access_unpack<ecolab::Plot>
  {
    template <class U>
    void operator()(classdesc::pack_t& p, const classdesc::string& d, U& a)
    {a.unpack(p);}
  };
}  

#include "plot.cd"
#ifdef _CLASSDESC
#pragma omit pack ecolab::Plot
#pragma omit unpack ecolab::Plot
#endif
#endif
