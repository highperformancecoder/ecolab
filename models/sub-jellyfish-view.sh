#! /bin/bash
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

i=7
while [ $i -lt 17 ]; do
  qsub -q single -N lake$i -l walltime=0:5:0 <<EOF
cd \$PBS_O_WORKDIR
lrun -n 1 jellyfish jellyfish-view.tcl lakes/OTM lake$i
EOF
    i=$[$i+1]
    done
