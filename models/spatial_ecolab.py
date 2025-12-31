from ecolab_model import spatial_ecolab as ecolab

from random import random, seed as randomSeed

from ecolab import array_urand, myid, device

print(device())

# we want initialisation to be identical across all processes
randomSeed(10)

# we want the array operations to have a different seed across processes
array_urand.seed(10+myid())

# initial number of species
nsp=10

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

numX=2
numY=2
ecolab.setGrid(numX,numY)
ecolab.partitionObjects()

print("initialising density")
for i in range(numX):
    for j in range(numY):
        #ecolab.cell(i,j).density([int(100*random()) for i in range(nsp)])
        ecolab.cell(i,j).density(nsp*[100])
print("density initialised")
        
ecolab.repro_rate(randomList(nsp, ecolab.repro_min(), ecolab.repro_max()))
ecolab.interaction.diag(randomList(nsp, -1e-3, -1e-3))
ecolab.random_interaction(3,0)
              
ecolab.interaction.val(randomList(len(ecolab.interaction.val), ecolab.odiag_min(), ecolab.odiag_max()))

ecolab.mutation(nsp*[ecolab.mut_max()])
ecolab.migration(nsp*[1e-1])
                  
from plot import plot
from GUI import gui, statusBar, windows

epoch=2000000
mut_factor=1000

extinctions=0
migrations=0
def stepImpl():
    #ecolab.setDensitiesDevice()
    ecolab.generate(100)
    ecolab.mutate()

    epochTs=ecolab.tstep()%epoch
    if (epochTs==0):
        ecolab.migration([x*mut_factor for x in ecolab.migration()])
    if (epochTs==epoch//2):
        ecolab.migration([x/mut_factor for x in ecolab.migration()])

    global extinctions, migrations
    migrations+=ecolab.migrate()
    extinctions+=ecolab.condense()
    #ecolab.syncThreads()
    #print(ecolab.nsp()())
    #ecolab.setDensitiesShared()
    #ecolab.gather()

print(ecolab.nsp()())
ecolab.makeConsistent()
ecolab.syncThreads()
print(ecolab.nsp()())

from timeit import timeit
print(timeit('stepImpl()', globals=globals(), number=10))
                
def step():
    global extinctions,migrations
    extinctions=0
    migrations=0
    for i in range(epoch//10000):
        stepImpl()
    print('migrations=',migrations,' extinctions=',extinctions)
    if myid()==0:
        nsp=len(ecolab.species)
        statusBar.configure(text=f't={ecolab.tstep()} nsp:{nsp}')
        plot('No. species',ecolab.tstep(),nsp,200*(ecolab.tstep()%epoch<0.5*epoch))
        #plot('No. species',ecolab.tstep(),nsp)
        plot('No. species by cell',ecolab.tstep(),ecolab.nsp()())
        plot('Extinctions',ecolab.tstep(),extinctions)
        plot('Migration',ecolab.tstep(),migrations)
#        for i in range(numX):
#            for j in range(numY):
#                plot(f'Density({i},{j})',ecolab.tstep(),ecolab.cell(i,j).density(), pens=ecolab.species())

gui(step)



