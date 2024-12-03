from ecolab_model import spatial_ecolab
# spatial_ecolab is a smart ptr to possible Device accessible memery, so must be dereferenced
ecolab=spatial_ecolab()

from random import random, seed as randomSeed

from ecolab import array_urand, myid, device

# we want initialisation to be identical across all processes
randomSeed(1)

# we want the array operations to have a different seed across processes
array_urand.seed(10+myid())

# initial number of species
nsp=100

ecolab.repro_min(-0.1)
ecolab.repro_max(0.1)
ecolab.odiag_min(-1e-5)
ecolab.odiag_max(1e-5)
#ecolab.mut_max(1e-4)
ecolab.mut_max(1e-3)
ecolab.sp_sep(0.1)

def randomList(num, min, max):
    return [random()*(max-min)+min for i in range(num)]

ecolab.species(range(nsp))

numX=8
numY=8
ecolab.setGrid(numX,numY)
ecolab.partitionObjects()
for i in range(numX):
    for j in range(numY):
        ecolab.cell(i,j).density(nsp*[100])

ecolab.repro_rate(randomList(nsp, ecolab.repro_min(), ecolab.repro_max()))
ecolab.interaction.diag(randomList(nsp, -1e-3, -1e-3))
ecolab.random_interaction(3,0)
              
ecolab.interaction.val(randomList(len(ecolab.interaction.val), ecolab.odiag_min(), ecolab.odiag_max()))

ecolab.mutation(nsp*[ecolab.mut_max()])
ecolab.migration(nsp*[1e-5])
                  
from plot import plot
from GUI import gui, statusBar, windows

print(device())

def step():
    ecolab.generate(100)
    ecolab.mutate()
    ecolab.migrate()
    ecolab.condense()
    ecolab.gather()
    if myid()==0:
        nsp=len(ecolab.species)
        statusBar.configure(text=f't={ecolab.tstep()} nsp:{nsp}')
        plot('No. species',ecolab.tstep(),nsp)
        plot('No. species by cell',ecolab.tstep(),ecolab.nsp()())
        for i in range(numX):
            for j in range(numY):
                plot(f'Density({i},{j})',ecolab.tstep(),ecolab.cell(i,j).density(), pens=ecolab.species())
    
gui(step)



