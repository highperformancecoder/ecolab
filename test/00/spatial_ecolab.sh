#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
. $here/test/common-test.sh


# if MPICH2, then we need to run an mpd
if [ -x `which mpd` ]; then
    mpd&
fi

cat >input.py <<EOF
from ecolab_model import spatial_ecolab as ecolab
from random import random

def randomList(num, min, max):
    return [random()*(max-min)+min for i in range(num)]

nsp=30
ecolab.species(range(nsp))
ecolab.repro_min(-.1)
ecolab.repro_max(.1)
ecolab.odiag_min(-1e-5)
ecolab.odiag_max(1e-5)
ecolab.repro_rate(randomList(nsp, ecolab.repro_min(), ecolab.repro_max()))
ecolab.interaction.diag(randomList(nsp, -1e-3, -1e-3))
ecolab.random_interaction(3,0)
ecolab.interaction.val(randomList(len(ecolab.interaction.val), ecolab.odiag_min(), ecolab.odiag_max()))

# mutation parameters
ecolab.mut_max(1e-4)
ecolab.sp_sep(0.1)
ecolab.mutation(nsp*[ecolab.mut_max()])

ecolab.migration(nsp*[1e-5])

ecolab.setGrid(2, 2)
initd=nsp*[100]
ecolab.cell(0,0).density(initd)
ecolab.cell(0,1).density(initd)
ecolab.cell(1,0).density(initd)
ecolab.cell(1,1).density(initd)
ecolab.distributeObjects()
ecolab.generate()
ecolab.gather()
# check densities have changed
assert ecolab.cell(0,0).density()!=initd
assert ecolab.cell(1,0).density()!=initd
assert ecolab.cell(0,1).density()!=initd
assert ecolab.cell(1,1).density()!=initd

# count individual before migrate
n0=nsp*[0]
for i in range(2):
    for j in range(2):
        d=ecolab.cell(i,j).density()
        for k in range(nsp):
            n0[k]+=d[k]

ecolab.migrate()
# count individuals after migrate
n1=nsp*[0]
for i in range(2):
    for j in range(2):
        d=ecolab.cell(i,j).density()
        for k in range(nsp):
            n1[k]+=d[k]


# check that individuals are conserved through migration
assert n0==n1
EOF

# try to find MPI implementation code was built with, and run
# mpivars.sh, since AEGIS strips off unusual LD_LIBRARY_PATHS
mpicxx=`which mpicxx`
mpibin=${mpicxx%/*}
if [ -f $mpibin/mpivars.sh ]; then 
    echo "$mpibin/mpivars.sh found"
    . $mpibin/mpivars.sh
fi

if which mpiexec; then
    mpiexec -n 2 python3 input.py
else
    python3 input.py
fi
if test $? -ne 0; then fail; fi

pass
