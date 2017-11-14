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
        logy(logy), scale(scale), o(logy? log10(o): o), o1(o1) {
      }
      double operator()(double y) const {
        return scale*((logy? log10(y): y)-o)+o1;
      }
    };
  private:    
    string m_image;
    std::vector<std::vector<double> > x;
    std::vector<std::vector<double> > y;
    //classdesc::shared_ptr<cairo::TkPhotoSurface> surface;
    cairo::SurfacePtr surface;
    


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

    bool inBounds(float x, float y) const {
      return x>=minx && x<=maxx && y>=miny && y<=maxy;
    }

    bool displayRHSscale() const {
      return !penSide.empty() && miny1<maxy1;
    }
  public:
    enum Side {left, right};
    enum PlotType {line, bar};

    Plot(): nxTicks(30), nyTicks(30), fontScale(1),
            offx(0), offy(0), logx(false), logy(false), 
            grid(false), subgrid(false), 
            leadingMarker(false), autoscale(true), 
            legend(false), legendSide(right), plotType(line), 
            minx(-1), maxx(1), miny(-1), maxy(1), miny1(1), maxy1(-1)
    {}
    virtual ~Plot() {}

    int nxTicks, nyTicks; ///< number of x/y-axis ticks
    double fontScale; ///< scale tick labels
    double offx, offy; ///< origin of plot within image
    bool logx, logy; ///< logarithmic plots (x/y axes)
    bool grid, subgrid; ///< draw grid options
    bool leadingMarker; ///< draw a leading marker on the curve (full draw only)
    bool autoscale; ///< autoscale plot to data
    bool legend;  ///< add a legend to the plot
    Side legendSide; ///< legend drawn towards the left or right
    PlotType plotType;
    /// axis labels
    string xlabel, ylabel, y1label;

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
    void redraw();
    /// draw the plot onto a given surface
    virtual void draw(cairo::Surface&);
    void draw(cairo_t*, double width, double height) const;
    /// clear all datapoints (and redraw)
    void clear();

    /// add a datapoint
    void add(unsigned pen, double x, double y) 
    {if (surface) add(*surface,pen,x,y);}
    void add(cairo::Surface&, unsigned pen, double x, double y);
    /// add multiple points (C=container - eg vector/array)
    template <class C> 
    void add(unsigned pen, const C& x, const C& y) 
    {if (surface) add(*surface,pen,x,y);}
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

    /// add multiple pens worth of data in one hit
    void add(const array_ns::array<unsigned>& pens, double x, 
             const array_ns::array<double>& y) 
    {if (surface) add(*surface,pens,x,y);}     
    void add(cairo::Surface& surf, 
                   const array_ns::array<unsigned>& pens, 
                   double x1, const array_ns::array<double>& y1);
    
    /// label a pen (for display in a legend). The string may contain
    /// pango markup.
    void labelPen(unsigned pen, const string& label);
    /// label a pen with an arbitrary graphic, given by \a label.
    void LabelPen(unsigned pen, const cairo::SurfacePtr& label);

    /// assign a side to display the scale for pen
    void AssignSide(unsigned pen, Side);

    //    void labelPen(TCL_args args) {LabelPen(int(args[0]), (char*)args[1]);}
    void assignSide(TCL_args args) {
      AssignSide(args[0], args[1].get<Side>());
    }

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

    // add a point to the graph withour redrwaing anything
    void addPt(unsigned pen, double x, double y);

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
