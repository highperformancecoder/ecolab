/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

#if defined(CAIRO)
#include "plot.h"
#include "cairo_base.h"
#include "ecolab_epilogue.h"
#include "pango.h"
#include <limits>
#include <fstream>
using namespace std;

extern "C" void ecolab_plot_link() {}

using ecolab::cairo::Colour;
namespace ecolab
{
  Colour palette[]=
    {
      {0,0,0,1}, //black
      {1,0,0,1}, //red
      {0,0.6,0,1}, //green
      {0,0,1,1}, //blue
      {1,0,1,1}, //magenta
      {0,1,1,1}, //cyan
      {1,0.647,0,1}, //orange
      {0.627,0.125,0.941,1} //purple
      
    };

  const int paletteSz = sizeof(palette)/sizeof(palette[0]);
}

namespace 
{
  // stroke the current path using the default linewidth
  void stroke(cairo_t* cairo)
  {
    cairo_save(cairo);
    cairo_identity_matrix(cairo);
    cairo_stroke(cairo);
    cairo_restore(cairo);
  }

  // displays a string represent ×10^n
  string orderOfMag(int n)
  {
    char label[20];
    if (abs(n)<=3)
      sprintf(label,"×%-6.*f",(n>0? 0: -n),pow(10.0,n));
    else
#ifdef PANGO
      sprintf(label,"×10<sup>%d</sup>",n); //pango markup used here
#else
      sprintf(label,"E%d",n); //fall back to basic Cairo text
#endif
    return label;
  }    

  // axis labels are either just the leading digit, or an order of
  // magnitude if log
  string logAxisLabel(double x)
  {
    char label[30];
    if (x<=0) return ""; // -ve values meaningless
        if (x>=0.01 && x<1)
          {
            sprintf(label,"%.2f",x);
          }
        else if (x>=1 && x<=100)
          {
            sprintf(label,"%.0f",x);
          }
        else
          {
            int omag=int(log10(x));
            int lead=x*pow(10,-omag);
            sprintf(label,"%d×10<sup>%d</sup>",lead,omag);
          }
        return label;
  }


  void showOrderOfMag(ecolab::Pango& pango, double scale, unsigned threshold)
  {
    scale=floor(log10(scale));
    if (abs(scale)>=threshold-1)
      {
        pango.setMarkup(orderOfMag(scale));
        pango.show();
       }
  }
  
  /*
    compute increment and tickoffset, returning the scale

    The idea is to get the tick marks to show integral values for plot neatness
  */
  void computeIncrementAndOffset
  (double minv, double maxv, int maxTicks, double& incr, double& offset)
  {
    double scale=pow(10.0, int(log10(max(abs(minv), abs(maxv)))));
    offset = scale * int(minv/scale); 
    incr = scale;
    double d=maxv-minv;
    while (d < 3*incr)
      incr *= 0.1;
    if (d > 5*maxTicks*incr)
      incr *= 10;
    else if (d > 2*maxTicks*incr)
      incr *= 5;
    else if (d > maxTicks*incr)
      incr *= 2;
  }

  class LogScale
  {
    int div; //=2, 5 or 10
    int scaleIncr;
    double scale;
  public:
    double operator()(unsigned i) {
      return (i%div+1)*pow(10,scaleIncr*(i/div))/div;
    }

    LogScale(double minv, double maxv, int maxTicks): scaleIncr(1)
    {
      scale=pow(10.0, int(log10(minv)));
      
      double s=log10(maxv/scale) / maxTicks;
      if (s>1)
        {
          scaleIncr = s+1;
          div=1;
        }
      else if (s>0.5)
        div=2;
      else
        div=5;
    }
  };
      
  // adjust y bounds to handle pathological situations
  void adjustMinyMaxy(double& miny, double& maxy)
  {
    double amm=abs(maxy)+abs(miny);
    if (amm>10*numeric_limits<double>::min())
      {
        double dy=maxy-miny;
        if (abs(dy)>=1e-2*amm)
          {
            // adjust to make sure there is some space around edges 
            miny -= 0.1*dy;
            maxy += 0.1*dy;
          }
        else if (miny<0)
          {
            // val -ve, set the graph from 2*val to 0
            miny*=2;
            maxy=0;
          }
        else if (miny>0)
          {
            // set the graph from 0 to 2*val
            miny=0;
            maxy*=2;
          }
      }
    else
      {
        // 0 constant function
        miny=-1;
        maxy=1;
      }
  }
}

namespace ecolab
{
  cairo_surface_t* Plot::cairoSurface() const {
      return surface? surface->surface(): 0;
  }
  int Plot::width() const {return surface? surface->width():0;}
  int Plot::height() const {return surface? surface->height():0;}


  string Plot::Image(const string& im, bool transparency)
  {
#if defined(TK)
    Tk_PhotoHandle photo = Tk_FindPhoto(interp(), im.c_str());
    if (photo)
      {
        surface.reset(new cairo::TkPhotoSurface(photo,transparency));
        cairo_surface_set_device_offset(surface->surface(),0.5*surface->width(),0.5*surface->height());
      }
    redraw();
#endif
    return m_image=im;
  }

  string Plot::axisLabel(double x, double scale, bool percent) const
  {
    char label[30];
    // change scale back to units
    int iscale=int(floor(log10(scale)));
    scale = pow(10.0, iscale);        
    if (unsigned(abs(iscale))+1>=exp_threshold)
      {
        int num=floor(x/scale+0.5);
        sprintf(label,"%d",num);
      }
    else
      {
        double num=scale*floor(x/scale+0.5);
        if (percent)
          sprintf(label,"%g%%",100*num);
        else
          sprintf(label,"%g",num);
      }

    return label;
  }

  void Plot::labelPen(unsigned pen, const string& label)
  {
    if (pen>=penTextLabel.size()) penTextLabel.resize(pen+1);
    penTextLabel[pen]=label;
  }

  void Plot::LabelPen(unsigned pen, const cairo::SurfacePtr& surf)
  {
    if (pen>=penLabel.size()) penLabel.resize(pen+1);
    penLabel[pen]=surf;
  }

  

  void Plot::assignSide(unsigned pen, Side side)
  {
    if (pen>=penSide.size()) penSide.resize(pen+1,left);
    penSide[pen]=side;
  }


  void Plot::setMinMax()
  {
    assert(x.size()==y.size());
    msg=NULL;
    
    // calculate min/max
    if (x.empty())
      {
        minx=logx? 0.1: -1;
        maxx=1;
        miny=logy? 0.1: -1;
        maxy=1;
      }
    else
      {
        minx=numeric_limits<double>::max();
        maxx=-numeric_limits<double>::max();
        miny=numeric_limits<double>::max();
        maxy=-numeric_limits<double>::max();
        miny1=numeric_limits<double>::max();
        maxy1=-numeric_limits<double>::max();
        
        for (size_t i=0; i<x.size(); ++i)
          {
            assert(x[i].size()==y[i].size());
            for (size_t j=0; j<x[i].size(); ++j)
              {
                if (x[i][j]<minx) minx=x[i][j];
                if (x[i][j]>maxx) maxx=x[i][j];
                if (i<penSide.size() && penSide[i]==right)
                  {
                    if (y[i][j]<miny1) miny1=y[i][j];
                    if (y[i][j]>maxy1) maxy1=y[i][j];
                  }
                else
                  {
                    if (y[i][j]<miny) miny=y[i][j];
                    if (y[i][j]>maxy) maxy=y[i][j];
                  }
              }
          }
      }
    if (!logx)
      {
        double dx=maxx-minx;
        minx = dx>0? minx-0.1*dx: -1;
        maxx = dx>0? maxx+0.3*dx: 1;
      }

    adjustMinyMaxy(miny,maxy);
    adjustMinyMaxy(miny1,maxy1);

  }

  void Plot::drawGrid
  (cairo_t* cairo, double tick, double increment, bool vertical, const XFY& xfy) const
  {
    // draw first subgrid if necessary
    double min=vertical? minx: miny;
    if (tick > min && tick-increment<=min) 
      drawGrid(cairo, tick-increment, increment, vertical, xfy);

    cairo_save(cairo);
    if (vertical)
      {
        cairo_move_to(cairo,iflogx(tick),xfy(miny));
        cairo_line_to(cairo,iflogx(tick),xfy(maxy));
      }
    else
      {
        cairo_move_to(cairo,iflogx(minx),xfy(tick));
        cairo_line_to(cairo,iflogx(maxx),xfy(tick));
      }

    cairo_save(cairo);
    cairo_set_source_rgba(cairo,0.5,0.5,0.5,1);
    cairo_identity_matrix(cairo);
    cairo_set_line_width(cairo,1);
    cairo_stroke(cairo);
    cairo_restore(cairo);

    if (subgrid && increment>0)
      {
        // subgrid lines done in grey
        if (vertical)
          {
            double dx=0.1*increment;
            for (double x=tick+dx; x<tick+increment; x+=dx)
              {
                cairo_move_to(cairo,iflogx(x),xfy(miny));
                cairo_line_to(cairo,iflogx(x),xfy(maxy));
              }
          }
        else
          {
            double dy=0.1*increment;
            for (double y=tick+dy; y<tick+increment; y+=dy)
              {
                cairo_move_to(cairo,iflogx(minx),xfy(y));
                cairo_line_to(cairo,iflogx(maxx),xfy(y));
              }
          }
        
        
        cairo_save(cairo);
        cairo_set_source_rgba(cairo,0.8,0.8,0.8,1);
        cairo_identity_matrix(cairo);
        cairo_set_line_width(cairo,1);
        cairo_stroke(cairo);
        cairo_restore(cairo);
      }

    cairo_restore(cairo);

  }

  void Plot::drawLegend(cairo_t* cairo, double w, double h) const
  {
    double dx=maxx-minx, dy=maxy-miny;
    double xoffs;
    // compute width of labels
    double width=0, height=0;
    double fy=0.03*fontScale*h;
    double yoffs=0.95*h-0.8*fy;

    switch (legendSide)
      {
      case left:
        xoffs=0.1*w;
        break;
      case right:
        xoffs=0.9*w-width;
        break;
      case boundingBox:
        xoffs=legendLeft*w;
        fy=legendFontSz*h;
        yoffs=legendTop*h-0.8*fy;
        break;
      }

    cairo::CairoSave cs(cairo);
    cairo_translate(cairo,0, h);
    cairo_scale(cairo,1,-1);

    for (size_t i=0; i<x.size(); ++i)
      if (i<penLabel.size() && penLabel[i])
        {
          height += 1.3*penLabel[i]->height();
          width = max(width, penLabel[i]->width());
        }
      else if (i<penTextLabel.size() && !penTextLabel[i].empty())
        {
          Pango p2(cairo);
          p2.setFontSize(fabs(fy));
          p2.setMarkup(penTextLabel[i]);
          height += 1.5*fy; //1.3*(p2.height());
          width = max(width, p2.width());
        }
    double labeloffs=xoffs+0.06*w;
    
    if (height>0)
      {
        cairo::CairoSave cs(cairo);
        cairo_rectangle(cairo,xoffs,yoffs+0.8*fy,width+0.06*w,-height);
        cairo_set_source_rgb(cairo,0,0,0);
        cairo_stroke_preserve(cairo);
        cairo_set_source_rgba(cairo,1,1,1,0.7);
        cairo_fill(cairo);
      }
    
    for (size_t i=0; i<x.size(); ++i)
      if (i<penLabel.size() && penLabel[i])
        {
          Colour& colour=palette[i%paletteSz];
          cairo_set_source_rgba(cairo, colour.r, colour.g, colour.b, colour.a);
          cairo_move_to(cairo, xoffs, yoffs);
          cairo_rel_line_to(cairo, 0.05*dx, 0);
          stroke(cairo);

          cairo::Surface& surf=*penLabel[i];

          /// translate and scale label graphic to fit where we want to put it
          cairo_pattern_t* pat=cairo_pattern_create_for_surface(surf.surface());
          cairo_matrix_t matrix;
          cairo_pattern_get_matrix(pat,&matrix);
          cairo_matrix_translate(&matrix, -surf.left(), surf.top());
          cairo_matrix_translate(&matrix, -labeloffs, -yoffs-0.5*surf.height());
          cairo_pattern_set_matrix(pat,&matrix);
          cairo_set_source(cairo, pat);

          // clip and copy pattern into the label location
          cairo_rectangle(cairo, labeloffs, 
                          yoffs-0.5*surf.height(), 
                          surf.width(), 
                          surf.height());
          cairo_fill(cairo);
          cairo_pattern_destroy(pat);

          // advance to next label
          yoffs-=1.5*surf.height();

        }
      else if (i<penTextLabel.size() && !penTextLabel[i].empty())
        {
          // render directly from text label
          Colour& colour=palette[i%paletteSz];
          cairo_set_source_rgba(cairo, colour.r, colour.g, colour.b, colour.a);

          cairo_move_to(cairo, xoffs, yoffs);
          cairo_rel_line_to(cairo, 0.05*w, 0);
          stroke(cairo);

          Pango p2(cairo);
          p2.setFontSize(fy);
          p2.setMarkup(penTextLabel[i]);
          cairo_move_to(cairo, labeloffs, yoffs+0.8*fy);
          p2.show();
          // advance to next label
          yoffs-=1.5*fy;
        }
  }

  void Plot::labelAxes(cairo_t* cairo, double width, double height) const
  {
    double dx=maxx-minx, dy=maxy-miny;
    
    // axis label font size (in pixels)
    double lh=0.05*fontScale*(height-2*offy)/(1+0.03*fontScale);
    // offsets to allow space for axis labels
    double loffx=lh*!ylabel.empty(), 
      loffx1=lh*!y1label.empty(), 
      loffy=lh*!xlabel.empty();


    double sx=(width-offx-loffx-loffx1)/dx, 
      sy=(height-2*offy-loffy)/dy;

    // work out the font size we should use
    double fontSz=0.03*fontScale;
    double fx=0, fy=fontSz*dy;
    cairo_user_to_device_distance(cairo,&fx,&fy);

    cairo_save(cairo);
    cairo_translate(cairo,0.5*width,0.5*height);
    Pango pango(cairo);
    if (!xlabel.empty())
      {
        pango.setFontSize(0.6*lh);
        pango.setMarkup(xlabel);
        cairo_move_to(cairo,-0.5*pango.width(),0.5*height-pango.height());
        pango.show();
      }
    
    if (!ylabel.empty())
      {
        pango.setFontSize(0.6*lh);
        pango.setMarkup(ylabel);
        cairo_move_to(cairo,offx-0.5*width,0.5*pango.width());
        pango.angle=-0.5*M_PI;
        pango.show();
      }
    
    if (!y1label.empty())
      {
        pango.setFontSize(0.6*lh);
        pango.setMarkup(y1label);
        cairo_move_to(cairo,0.5*width-pango.height(),0.5*pango.width());
        pango.show();
      }
    cairo_restore(cairo);
  }          

  void Plot::draw(cairo::Surface& surface)
  {
    cairo_identity_matrix(surface.cairo());
    cairo_translate(surface.cairo(),-0.5*surface.height(),-0.5*surface.width());
    if (autoscale) setMinMax();
    // if there is no size, set size to 1, so at least a bounding box is drawn
    if (maxx<=minx) maxx=minx+1;
    if (maxy<=miny) 
      {
        if (miny1<maxy1)
          {
            miny=miny1;
            maxy=maxy1;
          }
        else
          maxy=miny+1;
      }
    draw(surface.cairo(), surface.width(), surface.height());
  }

  double Plot::lh(double width, double height) const
  {
    return 0.05*fontScale*(height-2*offy)/(1+0.03*fontScale);
  }

  void Plot::draw(cairo_t* cairo, double width, double height) const
  {  
#if defined(CAIRO)

    const char* errMsg=NULL;
    if (logx && minx<=0)
      errMsg = "logx requires positive range";
    else if (logy && (miny<=0 || (displayRHSscale() && miny1<=0)))
      errMsg = "logy requires positive range";
    // ! used here, rather than reverse comparison to allow for NaNs, which always compare false
    else if (!(maxx>minx) || !(maxy>miny) || (displayRHSscale() && !(maxy1>miny1)))
      errMsg = "no data";

    if (msg || errMsg)
      {
        Pango pango(cairo);
        pango.setMarkup(errMsg? errMsg: msg);
        cairo_move_to(cairo,0.5*(width-pango.width()),0.5*(height-pango.height()));
        pango.show();
        return;
      }
      
    double dx=maxx-minx, dy=maxy-miny, dy1=maxy1-miny1;
    dx=iflogx(maxx)-iflogx(minx);
    dy=iflogy(maxy)-iflogy(miny);
    dy1=iflogy(maxy1)-iflogy(miny1);
    
    // axis label font size (in pixels)
    // offsets to allow space for axis labels
    double loffx=lh(width,height)*!ylabel.empty(), 
      loffx1=lh(width,height)*!y1label.empty(), 
      loffy=lh(width,height)*!xlabel.empty();
    

    
    labelAxes(cairo,width,height);

    width-=offx+loffx+loffx1;
    height-=2*offy+loffy;
    double sx=width/dx, sy=height/dy;
    double rhsScale = dy/dy1;

    {
      cairo::CairoSave cs(cairo);
      cairo_translate(cairo, offx+loffx-iflogx(minx)*sx, height);

      // NB do not use cairo scaling in y direction, but rather manually scale before passing to cairo.
      // See ticket #693
      cairo_scale(cairo, sx, -1);
      cairo_new_path(cairo);

      cairo_set_source_rgba(cairo, 0,0,0,1); //black
      cairo_rectangle(cairo,iflogx(minx),0,dx,height);
      cairo_clip_preserve(cairo);
      stroke(cairo);
      

      cairo_set_source_rgba(cairo, 0, 0, 0, 1);
      double xtickIncrement, xtick;

      if (xtickIncrement<0) return; //avoid infinte loop

      // work out the font size we should use
      double fontSz=0.02*fontScale;
      Pango pango(cairo);
      pango.setFontSize(fontSz*height);
      XFY aff(logy, sy, miny, 0); //manual affine transform - see ticket #693
      if (xticks.size())
        {
          unsigned startTick=0, endTick;
          for (startTick=0; startTick<xticks.size() && xticks[startTick].first < minx; startTick++);
          for (endTick=startTick; endTick<xticks.size() && xticks[endTick].first < maxx; endTick++);
          unsigned tickIncr=(endTick-startTick)/nxTicks+1;
          double xtick=0, incr=0;
          pango.angle=xtickAngle*M_PI/180.0;
          for (unsigned i=startTick; i<endTick; i+=tickIncr)
            {
              auto& xt=xticks[i];
              xtick=logx? log10(xt.first): xt.first;
              cairo_new_path(cairo);
              cairo_move_to(cairo,xtick,0);
              cairo_line_to(cairo,xtick,fontSz*height);
              stroke(cairo);
              cairo_move_to(cairo,xtick,fontSz*height*2);
              pango.setMarkup(xt.second);
              pango.show();
              if (grid)
                {
                  if (i<xticks.size()-tickIncr)
                    {
                      double nextX=xticks[i+tickIncr].first;
                      if (logx) nextX=log10(nextX);
                      incr=fabs(nextX-xtick);
                    }
                  drawGrid(cairo, xtick, incr, true, aff);
                }
            }
          pango.angle=0;
          if (grid && incr) // extend grid all the way
            while ((xtick+=incr)<maxx)
              drawGrid(cairo, xtick, incr, true, aff);
        }
      else if (logx)
        {
          LogScale ls(minx, maxx, nxTicks);
          int i=0;
          for (xtick=ls(0); xtick<maxx; i++, xtick=ls(i))
            if (xtick>=minx)
              {
                pango.setMarkup(logAxisLabel(xtick));
                cairo_new_path(cairo);
                cairo_move_to(cairo,log10(xtick),0);
                cairo_line_to(cairo,log10(xtick),fontSz*height);
                stroke(cairo);
                cairo_move_to(cairo,log10(xtick),fontSz*height*2);
                pango.show();
              
                if (grid)
                  drawGrid(cairo, xtick, ls(i+1)-xtick, true, aff);
              }
        }
      else
        {
          computeIncrementAndOffset(minx, maxx, nxTicks, xtickIncrement, xtick);
          cairo_move_to(cairo, maxx-0.05*dx*(1+fontScale), aff(miny+0.04*dy*(1+fontScale)));
          showOrderOfMag(pango, xtickIncrement, exp_threshold);

          for (; xtick<maxx; xtick+=xtickIncrement)
            if (xtick>=minx)
              {
                pango.setMarkup(axisLabel(xtick,xtickIncrement));
              
                cairo_new_path(cairo);
                cairo_move_to(cairo,xtick,aff(miny));
                cairo_line_to(cairo,xtick,aff(miny)+fontSz*height);
                stroke(cairo);
                cairo_move_to(cairo,xtick,aff(miny)+fontSz*height*2);
                pango.show();
              
                if (grid)
                  drawGrid(cairo, xtick, xtickIncrement, true, aff);
              }
        }
    
      double rightMargin=0.02*dx;

      if (logy)
        {
          LogScale ls(miny, maxy, nyTicks);
          int i=0;
          for (double ytick=ls(0); ytick<maxy; i++, ytick=ls(i))
            if (aff(ytick)>=fontSz*height)
              {
                pango.setMarkup(logAxisLabel(ytick));
                cairo_new_path(cairo);
                cairo_move_to(cairo,iflogx(minx),aff(ytick));
                cairo_line_to(cairo,iflogx(minx)+fontSz*dx,aff(ytick));
                stroke(cairo);
                cairo_move_to(cairo,iflogx(minx),aff(ytick));
                pango.show();
                if (grid)
                  drawGrid(cairo, ytick, ls(i+1)-ytick, false, aff);
              }
          if (displayRHSscale())
            {
              LogScale ls(miny1, maxy1, nyTicks);
              for (double ytick=ls(i=0); ytick<maxy1; i++, ytick=ls(i))
                if (aff(ytick)>=fontSz*height)
                  {
                    pango.setMarkup(logAxisLabel(ytick));
                  
                    cairo_new_path(cairo);
                    double yt=aff((ytick-miny1)*rhsScale+miny);
                    cairo_move_to(cairo,maxx,yt);
                    cairo_line_to(cairo,minx+0.95*dx,yt);
                    stroke(cairo);
                    cairo_move_to(cairo,maxx-(pango.width()*fontSz*dx)/pango.height()-rightMargin,yt);
                    pango.show();
                  }
            }
        }
      else
        {
          double ytickIncrement, ytick;
          computeIncrementAndOffset(miny, maxy, nyTicks, ytickIncrement, ytick);
          if (ytickIncrement<0) return; //avoid infinte loop

          cairo_move_to(cairo, minx+0.01*dx, aff(maxy));
          showOrderOfMag(pango, ytickIncrement, exp_threshold);
          
          for (; ytick<maxy; ytick+=ytickIncrement)
            if (aff(ytick)>=fontSz*height)
              {
                pango.setMarkup(axisLabel(ytick,ytickIncrement,percent));

                cairo_new_path(cairo);
                cairo_move_to(cairo,iflogx(minx),aff(ytick));
                cairo_line_to(cairo,iflogx(minx)+fontSz*dx,aff(ytick));
                stroke(cairo);
                cairo_move_to(cairo,iflogx(minx),aff(ytick));
                pango.show();
                if (grid)
                  drawGrid(cairo, ytick, ytickIncrement, false, aff);
              }

          if (displayRHSscale())
            {
              // draw scale on right hand side
              computeIncrementAndOffset(miny1, maxy1, nyTicks, ytickIncrement, ytick);

              cairo_move_to(cairo, maxx-(pango.width()*fontSz*dx)/pango.height()-rightMargin, aff(maxy));
              showOrderOfMag(pango, ytickIncrement, exp_threshold);

              for (; ytick<maxy1; ytick+=ytickIncrement)
                if (ytick>=miny1+fontSz*dy1)
                  {
                    pango.setMarkup(axisLabel(ytick,ytickIncrement,percent));
                  
                    cairo_new_path(cairo);
                    double yt=aff((ytick-miny1)*rhsScale+miny);
                    cairo_move_to(cairo,maxx,yt);
                    cairo_line_to(cairo,minx+0.95*dx,yt);
                    stroke(cairo);
                    cairo_move_to(cairo,maxx-(pango.width()*fontSz*dx)/pango.height()-rightMargin,yt);
                    pango.show();
                  }
            }
        }
    
      if (!x.empty())
        for (size_t i=0; i<x.size(); ++i)
          {
            Colour& colour=palette[i%paletteSz];

            // transform y coordinates (handles RHS being a different scale)
            XFY xfy=aff;
            Side side=left;
            if (i<penSide.size() && penSide[i]==right)
              {
                xfy.scale*=rhsScale;
                xfy.o=miny1;
                side=right;
              }

            if (x[i].size()>1)
              {
                switch (plotType)
                  {
                  case line:
                    cairo_set_source_rgba(cairo, colour.r, colour.g, colour.b, colour.a);
          
                    cairo_new_path(cairo);
                    cairo_move_to(cairo, iflogx(x[i][0]), xfy(y[i][0]));
                    for (size_t j=1; j<x[i].size(); ++j)
                      if (inBounds(x[i][j-1], y[i][j-1], side) && inBounds(x[i][j], y[i][j], side))
                        cairo_line_to(cairo, iflogx(x[i][j]), xfy(y[i][j]));
                      else
                        cairo_move_to(cairo, iflogx(x[i][j]), xfy(y[i][j]));

                    if (leadingMarker)
                      cairo_rectangle
                        (cairo, iflogx(x[i].back()), xfy(y[i].back()), 
                         0.01*dx,  0.01*dy*sy);
                    stroke(cairo);
                    break;
                  case bar:
                    {
                      // make bars translucent - see Minsky ticket #893
                      cairo_set_source_rgba(cairo, colour.r, colour.g, colour.b, 0.5*colour.a);
                      size_t j=0;
                      float w = abs(iflogx(x[i][1]) - iflogx(x[i][0]));
                      if (inBounds(iflogx(x[i][j]), y[i][j], side))
                        {
                          cairo_rectangle(cairo, iflogx(x[i][0])-0.5*w, 0, w, 
                                          xfy(y[i][0]));
                          cairo_fill(cairo);
                        }
                      for (++j; j<x[i].size()-1; ++j)
                        if (inBounds(iflogx(x[i][j]), y[i][j], side))
                          {
                            w=min(abs(iflogx(x[i][j]) - iflogx(x[i][j-1])), 
                                  abs(iflogx(x[i][j+1])-iflogx(x[i][j])));
                            cairo_rectangle(cairo, iflogx(x[i][j])-0.5*w, 0, w, 
                                            xfy(y[i][j]));
                            cairo_fill(cairo);
                          }
                      if (inBounds(iflogx(x[i][j]), y[i][j], side))
                        {
                          w=abs(iflogx(x[i][j]) - iflogx(x[i][j-1]));
                          cairo_rectangle(cairo, iflogx(x[i][j])-0.5*w, 0, w, 
                                          xfy(y[i][j]));
                          cairo_fill(cairo);
                        }
                    }
                  }
              }
          }
    }
    if (legend) drawLegend(cairo,width,height);
#endif    
  }
  
  void Plot::redraw()
  {
#if defined(CAIRO)
    if (surface)
      {
        surface->clear();
        draw(*surface);
        surface->blit();
      }
#endif
  }
        
  void Plot::clear()
  {
    x.clear(); y.clear();
    redraw();
  }

  void Plot::addNew(cairo::Surface& surf, bool doRedraw, 
                    const unsigned *startPen, const unsigned *finishPen, 
                    int numPt)
  {
    if (doRedraw) 
      {
        surf.clear(); 
        draw(surf);
      }
    else
      {
        cairo_save(surf.cairo());
        cairo_t* cairo=surf.cairo();   

        double dx=maxx-minx, dy=maxy-miny, dy1=maxy1-miny1;
    
        // axis label font size (in pixels)
        double lh=0.05*fontScale*(surf.height()-2*offy)/(1+0.03*fontScale);
        double loffx=lh*!ylabel.empty(), 
          loffx1=lh*!y1label.empty(), 
          loffy=lh*!xlabel.empty();
        
        double sx=(surf.width()-offx-loffx-loffx1)/dx, 
          sy=(surf.height()-2*offy-loffy)/dy;
        double rhsScale = dy/dy1;

        cairo_identity_matrix(cairo);
        cairo_reset_clip(cairo);
        cairo_translate(cairo, offx+loffx-minx*sx, maxy*sy);
        cairo_scale(cairo, sx, -sy);
        for (const unsigned* pen=startPen; pen<finishPen; ++pen)
          {
            Colour& colour=palette[*pen%paletteSz];
            cairo_set_source_rgba(cairo, colour.r, colour.g, colour.b, colour.a);
            
            // transform y coordinates (handles RHS being a different scale)
            XFY xfy;
            if (*pen<penSide.size() && penSide[*pen]==right)
              xfy=XFY(logy, rhsScale, miny1, miny);
            else
              xfy=XFY(logy, 1, 0, 0);

            unsigned endPt=x[*pen].size(), startPt=endPt-numPt;
            if (startPt>1)
              {
                cairo_new_path(cairo);
                cairo_move_to(cairo, iflogx(x[*pen][startPt-1]), 
                              xfy(y[*pen][startPt-1]));
                for (unsigned p=startPt; p<endPt; ++p)
                  cairo_line_to(cairo, iflogx(x[*pen][p]), xfy(y[*pen][p]));
                stroke(cairo);
              }
          }
        cairo_restore(surf.cairo());
      }
    if (&surf==surface.get())
      surface->blit();
  }

  void Plot::addPt(unsigned pen, double xx, double yy)
  {
    if (pen>=x.size())
      {
        x.resize(pen+1);
        y.resize(pen+1);
      }
    x[pen].push_back(xx);
    y[pen].push_back(yy);
  }

  void Plot::add(cairo::Surface& surf, unsigned pen, 
                 double x1, double y1)
  {
    addPt(pen,x1,y1);

    addNew(surf, x1<minx || x1>maxx || y1<miny || y1>maxy, &pen, &pen+1, 1);
  }

  void Plot::add(cairo::Surface& surf, const array<unsigned>& pens, 
                 double x1, const array<double>& y1)
  {
    bool doRedraw=x1<minx||x1>maxx;
    unsigned maxPen=max(pens);
    if (maxPen>=x.size())
        {
          x.resize(maxPen+1);
          y.resize(maxPen+1);
        }
      for (size_t p=0; p<y1.size(); ++p)
        {
          unsigned pen=pens[p];
          double yy=y1[pen];
          doRedraw|=yy<miny || yy>maxy;
          x[pen].push_back(x1);
          y[pen].push_back(yy);
        }
      addNew(surf, doRedraw, pens.begin(), pens.end(), 1);
    }      

  void Plot::plot(TCL_args args)
  {
    if (args.count>1) 
      {
        double x=args;
        array_ns::array<double> y; args>>y;
        assert(args.count==0);
        array_ns::array<unsigned> pens=pcoord(y.size());
        add(pens,x,y);
      }
  }

  void Plot::exportAsCSV(const std::string& filename, const string& separator) const
  {
    ofstream of(filename);
    of<<"#"<<ylabel<<" by "<<xlabel<<endl;
    of<<"#";
    if (!xticks.empty())
      of<<"labelx"<<separator<<"label"<<separator;
    for (size_t i=0; i<x.size(); ++i)
      of << (i>0? separator:"")<<"x"<<i<<separator<<"y"<<i;
    of<<endl;
    size_t maxxsize=xticks.size();
    for (size_t i=0; i<x.size(); ++i) maxxsize=max(maxxsize, x[i].size());

    for (size_t i=0; i<maxxsize; ++i)
      {
        if (!xticks.empty())
          of<<xticks[i].first<<separator<<xticks[i].second<<separator;
        for (size_t j=0; j<x.size(); ++j)
          {
            if (j>0) of<<separator;
            if (i<x[j].size() && i<y[j].size() && isfinite(y[j][i]))
              of << x[j][i]<<separator<<y[j][i];
            else
              of << separator;
          }
        of<<endl;
      }
  }

  
  TCLTYPE(Plot);

}


#endif
