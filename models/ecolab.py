from ecolab_model import ecolab, Plot
ecolab.species([1,2])
ecolab.density([100,100]) 
ecolab.create([0,0])
ecolab.repro_rate([.1,-.1])
ecolab.interaction.diag([-.0001,-1e-5])
ecolab.interaction.val([-0.001,0.001])
ecolab.interaction.row([0,1])
ecolab.interaction.col([1,0])
ecolab.mutation([.01,.01])
ecolab.sp_sep(.1)
ecolab.repro_min(-.1)
ecolab.repro_max(.1)
ecolab.odiag_min(-1e-3)
ecolab.odiag_max(1e-3)
ecolab.mut_max(.01)

Plot('plot')
from ecolab_model import plot

from tkinter import Tk,ttk
plotWindow=Tk()
plotWindow.eval('load ../lib/libecolab.so')
plotWindow.eval('image create cairoSurface plotCanvas -surface ecolab_model plot')
frame=ttk.Frame(plotWindow, width=500, height=500)
frame.pack()
label=ttk.Label(frame, image="plotCanvas")
label.pack()

#set palette {black red}

running=False
def simulate():
    running=True
    while running:
        ecolab.generate()
	#    .statusbar configure -text "t=[ecolab.tstep] n=[ecolab.density]"
        density=ecolab.density()._properties
        print(ecolab.tstep(), density)
        plot.addPt(0,ecolab.tstep(),density[0])
        plot.addPt(1,ecolab.tstep(),density[1])
        plot.requestRedraw()
        plotWindow.update()

simulate()



