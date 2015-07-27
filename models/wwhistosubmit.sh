#!/bin/bash
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

if [ $# -lt 1 ]; then 
  echo "please supply value of c"
  exit
else
  c=$1
fi

i=0
while [ $i -lt 50 ]; do 
   cat >wwhisto$i,c=$c.job <<EOF 
#PBS -lcput=4:0:0,walltime=4:0:0,mem=512MB,ncpus=1 -N wwh$i,c=$c
if [ ! -z "\$PBS_O_WORKDIR" ]; then cd \$PBS_O_WORKDIR; fi
#lrun -n 1 webworld wwhisto.tcl $i $c
webworld wwhisto.tcl $i $c
if [ -e wwhisto$i,c=$c.ckpt ]; then 
  qsub wwhisto$i,c=$c.job
  fi
EOF
  qsub wwhisto$i,c=$c.job
  i=$[$i+1]
  done
