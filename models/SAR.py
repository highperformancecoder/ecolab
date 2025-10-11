# run a single instance of a SAR ensemble run
from ecolab_model import spatial_ecolab as ecolab

from random import random, seed as randomSeed

from ecolab import array_urand, myid, device

import os
import sys
from math import sqrt

# we want initialisation to be identical across all processes
randomSeed(1)

# we want the array operations to have a different seed across processes
array_urand.seed(10+myid())


nsp=int(sys.argv[1])         # initial number of species
A=int(sys.argv[2])           # area
mut_max=float(sys.argv[3])   # mutation rate
migration=float(sys.argv[4]) # initial migration rate


ecolab.repro_min(-0.1)
ecolab.repro_max(0.1)
ecolab.odiag_min(-1e-5)
ecolab.odiag_max(1e-5)
ecolab.mut_max(mut_max)
ecolab.sp_sep(0.1)

def randomList(num, min, max):
    return [random()*(max-min)+min for i in range(num)]

ecolab.species(range(nsp))

numX=int(sqrt(A))
numY=A//numX
ecolab.setGrid(numX,numY)
ecolab.partitionObjects()

# initialises allocators and space on GPU, so that density arrays can be set
#ecolab.makeConsistent()

for i in range(numX):
    for j in range(numY):
        ecolab.cell(i,j).density(nsp*[100])
        
ecolab.repro_rate(randomList(nsp, ecolab.repro_min(), ecolab.repro_max()))
ecolab.interaction.diag(randomList(nsp, -1e-3, -1e-3))
ecolab.random_interaction(3,0)
              
ecolab.interaction.val(randomList(len(ecolab.interaction.val), ecolab.odiag_min(), ecolab.odiag_max()))

ecolab.mutation(nsp*[mut_max])
ecolab.migration(nsp*[migration])
                  
from plot import plot
from GUI import gui, statusBar, windows

def stepImpl():
    ecolab.generate(100)
    ecolab.mutate()
    ecolab.migrate()
    ecolab.condense()
    ecolab.gather()
    
def av(x):
    return sum(x)/len(x) if len(x)>0 else 0
    
def step():
    stepImpl()
    # area, mutation, migration, no. species, connectivity

for i in range(100):
    step()

print(numX*numY, av(ecolab.mutation()), av(ecolab.migration()), len(ecolab.species), sum([x*x for x in ecolab.interaction.val()])/len(ecolab.species)**2)



