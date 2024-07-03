from ecolab_model import model, Plot
model.species([1,2])
model.density([100,100]) 
model.create([0,0])
model.repro_rate([.1,-.1])
model.interaction.diag([-.0001,-1e-5])
model.interaction.val([-0.001,0.001])
model.interaction.row([0,1])
model.interaction.col([1,0])
model.mutation([.01,.01])
model.sp_sep(.1)
model.repro_min(-.1)
model.repro_max(.1)
model.odiag_min(-1e-3)
model.odiag_max(1e-3)
model.mut_max(.01)

Plot('plot')
from model_model import plot

from tkinter import Tk,ttk
plotWindow=Tk()
plotWindow.eval('load ../lib/libecolab.so')
plotWindow.eval('image create cairoSurface plotCanvas -surface model_model plot')
frame=ttk.Frame(plotWindow, width=500, height=500)
frame.pack()
label=ttk.Label(frame, image="plotCanvas")
label.pack()

#set palette {black red}

running=False
def simulate():
    running=True
    while running:
        model.generate()
	#    .statusbar configure -text "t=[model.tstep] n=[model.density]"
        density=model.density()._properties
        print(model.tstep(), density)
        plot.addPt(0,model.tstep(),density[0])
        plot.addPt(1,model.tstep(),density[1])
        plot.requestRedraw()
        plotWindow.update()

simulate()



