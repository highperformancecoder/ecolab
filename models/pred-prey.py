from ecolab_model import ecolab
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

from plot import plot
from GUI import gui, statusBar

def step():
    ecolab.generate()
    density=ecolab.density()._properties
    statusBar.configure(text=f't={ecolab.tstep()} n={density}')
    #print(ecolab.tstep(), density)
    #plot('density1',ecolab.tstep(),density[0],density[1])
    plot('density',ecolab.tstep(),density)

gui(step)



