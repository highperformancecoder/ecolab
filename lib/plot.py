import ecolab
from ecolab import Plot, ecolabHome
from tkinter import Tk,ttk
from GUI import windows
from weakref import ref

def plot(name,x,*y):
    plotName='plot#'+name
    if not plotName in ecolab.__dict__:
        Plot(plotName)
        plotWindow=Tk()
        plotWindow.eval('load '+ecolabHome()+'/lib/ecolab.so')
        plotWindow.eval('image create cairoSurface plotCanvas -surface ecolab '+plotName)
        label=ttk.Label(plotWindow, image="plotCanvas")
        label.pack()
        windows.append(ref(plotWindow))
    plot=ecolab.__dict__[plotName]
    if len(y)==1: y=y[0] # deal with an single array of values being passed
    for i in range(len(y)):
        plot.addPt(i, x, y[i])
    plot.requestRedraw()
