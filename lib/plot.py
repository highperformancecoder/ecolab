import ecolab
from ecolab import Plot, ecolabHome
from tkinter import Tk,ttk
from GUI import windows
from weakref import ref

def getPlot(name):
    plotName='plot#'+name.replace(' ','')
    if not plotName in ecolab.__dict__:
        Plot(plotName)
        plotWindow=Tk()
        plotWindow.wm_title(name)
        plotWindow.eval('load '+ecolabHome()+'/lib/ecolab.so')
        plotWindow.eval('image create cairoSurface plotCanvas -surface ecolab '+plotName)
        label=ttk.Label(plotWindow, image="plotCanvas")
        label.pack()
        windows.append(ref(plotWindow))
    return ecolab.__dict__[plotName]

def plot(name,x,*y):
    plot=getPlot(name)
    if hasattr(y,'__len__') and len(y)==1: y=y[0] # deal with an single array of values being passed
    if hasattr(y,'__len__'):
        for i in range(len(y)):
            plot.addPt(i, x, y[i])
    else:
        plot.addPt(0, x, y)
    plot.requestRedraw()

def penPlot(name,x,y,pens):
    assert len(y)==len(pens)
    plot=getPlot(name)
    for i in range(len(y)):
        plot.addPt(pens[i], x, y[i])
    plot.requestRedraw()
    
