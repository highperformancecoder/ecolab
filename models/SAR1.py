# run a single instance of a SAR ensemble run
from ecolab_model import spatial_ecolab as ecolab

from random import random, seed as randomSeed

from ecolab import array_urand, myid, device

import os
import sys
from math import sqrt

# we want initialisation to be identical across all processes
#randomSeed(1)

numReplicants=1000

# we want the array operations to have a different seed across processes
array_urand.seed(int(1024*random()))

def randomList(num, min, max):
    return [random()*(max-min)+min for i in range(num)]

def step():
    ecolab.generate(100)
    ecolab.mutate()
    ecolab.migrate()
    ecolab.condense()
    ecolab.gather()
    
def av(x):
    return sum(x)/len(x) if len(x)>0 else 0

migration=float(sys.argv[1])

# for this experiment, do not mutate mutation or migration rates
ecolab.fixMutation(True)
ecolab.fixMigration(True)

print("Replicate, Area, Max mutation, Init. Mig, Init. num sp, Mutation rate, Migration rate, Number of species, Interaction strength^2",flush=True)
for A in [1, 2, 4, 6, 9, 12, 16]:
    for mut_max in [1e-3]:
            for nsp in [100]:
                ecolab.tstep(0)
                ecolab.last_mig_tstep(0)
                ecolab.repro_min(-0.1)
                ecolab.repro_max(0.1)
                ecolab.odiag_min(-1e-5)
                ecolab.odiag_max(1e-5)
                ecolab.mut_max(mut_max)
                ecolab.sp_sep(0.1)
                
                ecolab.species(range(nsp))
                ecolab.create(nsp*[0])
                

                numX=int(sqrt(A))
                numY=A//numX
                ecolab.setGrid(numX,numY)
                ecolab.partitionObjects()

                for i in range(numX):
                    for j in range(numY):
                        ecolab.cell(i,j).density(nsp*[100])
        
                ecolab.repro_rate(randomList(nsp, ecolab.repro_min(), ecolab.repro_max()))
                ecolab.interaction.diag(randomList(nsp, -1e-3, -1e-3))
                ecolab.random_interaction(3,0)
              
                ecolab.interaction.val(randomList(len(ecolab.interaction.val), ecolab.odiag_min(), ecolab.odiag_max()))

                ecolab.mutation(nsp*[mut_max])
                ecolab.migration(nsp*[migration])
                  
                for i in range(1000):
                    step()

                for i in range(numReplicants):
                    print(i, A, mut_max, migration, nsp, av(ecolab.mutation()), av(ecolab.migration()), len(ecolab.species), ecolab.connectivity(), flush=True,sep=',')
                    for j in range(50):
                        step()


