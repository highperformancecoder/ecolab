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
        label.pack(fill='both',expand=True)
        windows.append(ref(plotWindow))
    return ecolab.__dict__[plotName]

def plot(name,x,*y,pens=[]):
    plot=getPlot(name)
    if hasattr(y,'__len__') and len(y)==1: y=y[0] # deal with an single array of values being passed
    if hasattr(y,'__len__'):
        for i in range(len(y)):
            plot.addPt(pens[i] if len(pens)>0 else i, x, y[i])
    else:
        plot.addPt(pens[0] if len(pens)>0 else 0, x, y)
    plot.requestRedraw()

    
