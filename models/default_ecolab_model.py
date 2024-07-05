from ecolab_model import ecolab
from random import random
# initial number of species
nsp=100

ecolab.repro_min(-0.1)
ecolab.repro_max(0.1)
ecolab.odiag_min(-1e-5)
ecolab.odiag_max(1e-5)
ecolab.mut_max(1e-4)
ecolab.sp_sep(0.1)

def randomList(num, min, max):
    return [random()*(max-min)+min for i in range(num)]

ecolab.species(range(nsp))
ecolab.density(nsp*[100])

ecolab.repro_rate(randomList(nsp, ecolab.repro_min(), ecolab.repro_max()))
ecolab.interaction.diag(randomList(nsp, -1e-3, -1e-3))
ecolab.random_interaction(3,0)
                  
ecolab.interaction.val(randomList(len(ecolab.interaction.val), ecolab.odiag_min(), ecolab.odiag_max()))

ecolab.mutation(nsp*[ecolab.mut_max()])
ecolab.migration(nsp*[1e-5])
                  
from plot import plot
from GUI import gui, statusBar

def step():
    ecolab.generate()
    ecolab.mutate()
    #ecolab.condense()
    nsp=len(ecolab.species)
    statusBar.configure(text=f't={ecolab.tstep()} nsp:{nsp}')
    plot('No. species',ecolab.tstep(),nsp,0)
    plot('Density',ecolab.tstep(),ecolab.density()._properties)

gui(step)



