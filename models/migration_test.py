from ecolab_model import spatial_ecolab as ecolab
from random import random

from ecolab import array_urand
array_urand.seed(10)

# initial number of species
nsp=100

ecolab.species(range(nsp))

numX=10
numY=10
ecolab.setGrid(numX,numY)
ecolab.partitionObjects()
ecolab.cell(4,4).density(nsp*[100])

ecolab.migration(nsp*[0.5])
                  
from plot import plot
from GUI import gui, statusBar, windows

ecolab.makeConsistent()
def step():
    ecolab.migrate()
    ecolab.tstep(ecolab.tstep()+1)
    plot('No. species by cell',ecolab.tstep(),ecolab.nsp()())
    plot(f'Density(4,4)',ecolab.tstep(),ecolab.cell(4,4).density(), pens=ecolab.species())
    plot(f'Density(2,2)',ecolab.tstep(),ecolab.cell(2,2).density(), pens=ecolab.species())
    
gui(step)



