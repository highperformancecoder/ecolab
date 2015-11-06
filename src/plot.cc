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

//  double round2Dplaces(double x)
//  {
//    if (x==0) return 0;
//    double b = pow(10.0, int(log10(fabs(x))-2));
//    return b*int(x/b);
//  }

  // displays a string represent 10^n
  string orderOfMag(int n)
  {
    char label[20];
    if (abs(n)<=3)
      sprintf(label,"%-6.*f",(n>0? 0: -n),pow(10.0,n));
    else
#ifdef PANGO
      sprintf(label,"10<sup>%d</sup>",n); //pango markup used here
#else
      sprintf(label,"1E%d",n); //fall back to basic Cairo text
#endif
    return label;
  }    

  // axis labels are either just the leading digit, or an order of
  // magnitude if log
  string axisLabel(double x, double scale, bool log)
  {
    char label[10];
    // change scale back to units
    int iscale=int(floor(log10(scale)));
    scale = pow(10.0, iscale);
    int num=floor(x/scale+0.5);

    if (log)
      {
        if (x<=0) return ""; // -ve values meaningless
        return orderOfMag(num+iscale);
      }
    else
      sprintf(label,"%d",num);
    return label;
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

  // transform y coordinates (handles RHS being a different scale)
  struct XFY
  {
    bool logy;
    double scale, o, o1;
    XFY() {}
    XFY(bool logy, double scale, double o, double o1): 
      logy(logy), scale(scale), o(o), o1(o1) {}
    double operator()(double y) {
      return scale*((logy? log10(y): y)-o)+o1;}
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

  

  void Plot::AssignSide(unsigned pen, Side side)
  {
    if (pen>=penSide.size()) penSide.resize(pen+1,left);
    penSide[pen]=side;
  }


  void Plot::setMinMax()
  {
    assert(x.size()==y.size());
    
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
    if (logx)
      {
        if (minx<=0)
          throw error("logarithmic xscale needs positive data");
        minx=log10(minx);
        maxx=log10(maxx);
      }
    else
      {
        double dx=maxx-minx;
        minx = dx>0? minx-0.1*dx: -1;
        maxx = dx>0? maxx+0.3*dx: 1;
      }
    if (logy)
      {
        if (miny<0 || (!penSide.empty() && miny1<0))
          throw error("logarithmic yscale needs non-negative data");
        if (miny==0)
          {
            // find next most minimum y
            miny=maxy;
            for (size_t i=0; i<x.size(); ++i)
              if (penSide.size()<=i || penSide[i]==left)
                for (size_t j=0; j<x[i].size(); ++j)
                  if (y[i][j]<miny && y[i][j]>0) miny=y[i][j];
          }
        if (!penSide.empty() && miny1==0)
          {
            // find next most minimum y
            miny1=maxy1;
            for (size_t i=0; i<x.size(); ++i)
              if (penSide.size()>i && penSide[i]==right)
                for (size_t j=0; j<x[i].size(); ++j)
                  if (y[i][j]<miny1 && y[i][j]>0) miny1=y[i][j];
          }
        miny=log10(miny);
        maxy=log10(maxy);
        miny1=log10(miny1);
        maxy1=log10(maxy1);
      }

    adjustMinyMaxy(miny,maxy);
    adjustMinyMaxy(miny1,maxy1);

  }

  void Plot::drawGrid
  (cairo_t* cairo, double tick, double increment, bool vertical) const
  {
    // draw first subgrid if necessary
    double min=vertical? minx: miny;
    if (tick > min && tick-increment<=min) 
      drawGrid(cairo, tick-increment, increment, vertical);

    cairo_save(cairo);
    if (vertical)
      {
        cairo_move_to(cairo,tick,miny);
        cairo_line_to(cairo,tick,maxy);
      }
    else
      {
        cairo_move_to(cairo,minx,tick);
        cairo_line_to(cairo,maxx,tick);
      }

    cairo_save(cairo);
    cairo_set_source_rgba(cairo,0.5,0.5,0.5,1);
    cairo_identity_matrix(cairo);
    cairo_set_line_width(cairo,1);
    cairo_stroke(cairo);
    cairo_restore(cairo);

    if (subgrid)
      {
        // subgrid lines done in grey
        if (vertical)
          {
            double dx=0.1*increment;
            for (double x=tick+dx; x<tick+increment; x+=dx)
              {
                cairo_move_to(cairo,x,miny);
                cairo_line_to(cairo,x,maxy);
              }
          }
        else
          {
            double dy=0.1*increment;
            for (double y=tick+dy; y<tick+increment; y+=dy)
              {
                cairo_move_to(cairo,minx,y);
                cairo_line_to(cairo,maxx,y);
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
    double xoffs, yoffs=0.95*h;
    // compute width of labels
    double width=0, height=0;
    double fy=0.03*h;

    cairo_save(cairo);
    cairo_identity_matrix(cairo);
    cairo_translate(cairo,-0.5*w, 0.5*h);
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
          height += 1.3*(p2.height());
          width = max(width, p2.width());
        }
    if (legendSide==left)
      xoffs=0.2*w;
    else
      xoffs=0.7*w;
    double labeloffs=xoffs+0.06*w;

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
    cairo_restore(cairo);
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
    cairo_identity_matrix(cairo);
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

  void Plot::draw(cairo_t* cairo, double width, double height) const
  {  
#if defined(CAIRO)

    double dx=maxx-minx, dy=maxy-miny, dy1=maxy1-miny1;
    
    // axis label font size (in pixels)
    double lh=0.05*fontScale*(height-2*offy)/(1+0.03*fontScale);
    // offsets to allow space for axis labels
    double loffx=lh*!ylabel.empty(), 
      loffx1=lh*!y1label.empty(), 
      loffy=lh*!xlabel.empty();
    

    double sx=(width-offx-loffx-loffx1)/dx, 
      sy=(height-2*offy-loffy)/dy;
    double rhsScale = dy/dy1;

    //cairo_reset_clip(cairo);
    cairo_translate(cairo, offx+loffx-minx*sx, maxy*sy);
    cairo_scale(cairo, sx, -sy);
    cairo_new_path(cairo);

    cairo_set_source_rgba(cairo, 0,0,0,1); //black
    cairo_rectangle(cairo,minx,miny,dx,dy);
    //cairo_clip_preserve(cairo);
    stroke(cairo);

    if (legend) drawLegend(cairo,width,height);
    labelAxes(cairo,width,height);

    // work out the font size we should use
    double fontSz=0.02*fontScale;
    double fx=0, fy=fontSz*dy;
    cairo_user_to_device_distance(cairo,&fx,&fy);

    cairo_set_source_rgba(cairo, 0, 0, 0, 1);
    double xtickIncrement, xtick;
    computeIncrementAndOffset(minx, maxx, nxTicks, xtickIncrement, xtick);

    if (xtickIncrement<0) return; //avoid infinte loop

    Pango pango(cairo);
    pango.setFontSize(fabs(fy));
    if (!logx) // display scale multiplier
      {
        cairo_move_to(cairo, maxx-0.05*dx*(1+fontScale), miny+0.04*dy*(1+fontScale));
        pango.setMarkup("x "+orderOfMag(floor(log10(xtickIncrement))));
        pango.show();
      }

    for (; xtick<maxx; xtick+=xtickIncrement)
      if (xtick>=minx)
        {
          pango.setMarkup(axisLabel(xtick,xtickIncrement,logx));
          
          cairo_new_path(cairo);
          cairo_move_to(cairo,xtick,miny);
          cairo_line_to(cairo,xtick,miny+fontSz*dy);
          stroke(cairo);
          cairo_move_to(cairo,xtick,miny+fontSz*dy*2);
          pango.show();

          if (grid)
            drawGrid(cairo, xtick, xtickIncrement, true);
        }

    double ytickIncrement, ytick;
    computeIncrementAndOffset(miny, maxy, nyTicks, ytickIncrement, ytick);
    if (ytickIncrement<0) return; //avoid infinte loop

    if (!logy) // display scale multiplier
      {
        cairo_move_to(cairo, minx+0.01*dx, maxy);
        pango.setMarkup("x "+orderOfMag(floor(log10(ytickIncrement))));
        pango.show();
      }

    for (; ytick<maxy; ytick+=ytickIncrement)
      if (ytick>=miny+fontSz*dy)
        {
          pango.setMarkup(axisLabel(ytick,ytickIncrement,logy));

          cairo_new_path(cairo);
          cairo_move_to(cairo,minx,ytick);
          cairo_line_to(cairo,minx+fontSz*dx,ytick);
          stroke(cairo);
          cairo_move_to(cairo,minx,ytick);
          pango.show();
          if (grid)
            drawGrid(cairo, ytick, ytickIncrement, false);
        }

    if (displayRHSscale())
      {
        // draw scale on right hand side
        computeIncrementAndOffset(miny1, maxy1, nyTicks, ytickIncrement, ytick);
        double rightMargin=0.02*dx;

        if (!logy) // display scale multiplier
          {
            pango.setMarkup("x "+orderOfMag(floor(log10(ytickIncrement))));
            cairo_move_to(cairo, maxx-(pango.width()*fontSz*dx)/pango.height()-rightMargin, maxy);
            pango.show();
          }

        for (; ytick<maxy1; ytick+=ytickIncrement)
          if (ytick>=miny1+fontSz*dy1)
            {
              pango.setMarkup(axisLabel(ytick,ytickIncrement,logy));

              cairo_new_path(cairo);
              double yt=(ytick-miny1)*rhsScale+miny;
              cairo_move_to(cairo,maxx,yt);
              cairo_line_to(cairo,minx+0.95*dx,yt);
              stroke(cairo);
              cairo_move_to(cairo,maxx-(pango.width()*fontSz*dx)/pango.height()-rightMargin,yt);
              pango.show();
        }
      }

    if (!x.empty())
      for (size_t i=0; i<x.size(); ++i)
        {
          Colour& colour=palette[i%paletteSz];
          cairo_set_source_rgba(cairo, colour.r, colour.g, colour.b, colour.a);
          
          // transform y coordinates (handles RHS being a different scale)
          XFY xfy;
          if (i<penSide.size() && penSide[i]==right)
            xfy=XFY(logy, rhsScale, miny1, miny);
          else
            xfy=XFY(logy, 1, 0, 0);

          if (x[i].size()>1)
            {
              switch (plotType)
                {
                case line:
                  cairo_new_path(cairo);
                  cairo_move_to(cairo, iflogx(x[i][0]), xfy(y[i][0]));
                  for (size_t j=0; j<x[i].size(); ++j)
                    if (inBounds(iflogx(x[i][j-1]), xfy(y[i][j-1])) && 
                        inBounds(iflogx(x[i][j]), xfy(y[i][j])))
                      cairo_line_to(cairo, iflogx(x[i][j]), xfy(y[i][j]));
                    else
                      cairo_move_to(cairo, iflogx(x[i][j]), xfy(y[i][j]));

                  if (leadingMarker)
                    cairo_rectangle
                        (cairo, iflogx(x[i].back()), xfy(y[i].back()), 
                         0.01*dx,  0.01*dy);
                  stroke(cairo);
                  break;
                case bar:
                  {
                    size_t j=0;
                    float w = abs(iflogx(x[i][1]) - iflogx(x[i][0]));
                    if (inBounds(iflogx(x[i][j]), xfy(y[i][j])))
                      {
                        cairo_rectangle(cairo, iflogx(x[i][0])-0.5*w, 0, w, 
                                        xfy(y[i][0]));
                        cairo_fill(cairo);
                      }
                    for (++j; j<x[i].size()-1; ++j)
                      if (inBounds(iflogx(x[i][j]), xfy(y[i][j])))
                        {
                          w=min(abs(iflogx(x[i][j]) - iflogx(x[i][j-1])), 
                                abs(iflogx(x[i][j+1])-iflogx(x[i][j])));
                          cairo_rectangle(cairo, iflogx(x[i][j])-0.5*w, 0, w, 
                                          xfy(y[i][j]));
                          cairo_fill(cairo);
                        }
                    if (inBounds(iflogx(x[i][j]), xfy(y[i][j])))
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


  TCLTYPE(Plot);

}


#endif
