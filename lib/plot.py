from ecolab import Plot, ecolabHome
from tkinter import Tk,ttk
import ecolab

plotWindows={}

def plot(name,x,*y):
    plotName='plot#'+name
    if not plotName in plotWindows:
        Plot(plotName)
        plotWindows[plotName]=Tk()
        plotWindows[plotName].eval('load '+ecolabHome()+'/lib/ecolab.so')
        plotWindows[plotName].eval('image create cairoSurface plotCanvas -surface ecolab '+plotName)
        frame=ttk.Frame(plotWindows[plotName], width=500, height=500)
        frame.pack()
        label=ttk.Label(frame, image="plotCanvas")
        label.pack()
    plot=ecolab.__dict__[plotName]
    if len(y)==1: y=y[0] # deal with an single array of values being passed
    for i in range(len(y)):
        plot.addPt(i, x, y[i])
    plot.requestRedraw()
    plotWindows[plotName].update()
