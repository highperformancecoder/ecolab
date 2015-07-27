#!/bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

#PBS -lnodes=1 -lcput=0:30:0
#\
cd $PBS_O_WORKDIR
#\
exec time lrun jellyfish jellyfish-batch.tcl 
source lakes/OTM

set time 7
set day 1
set month 0
set latitude 7

parallel lake.restart [file rootname $lakef].bin
parallel lake.mangrove_height [lake.mangrove_height]
parallel lake.uni.seed {[expr 100*([myid]+1)]}
parallel gaussrand gran
parallel gran.uni.seed {[expr 100*([myid]+1)]}
parallel lake.speed_dist.gen gran
parallel lake.angle_dist.gen gran
parallel lake.photo_taxis .1
# turning probability
parallel lake.change_prob .8
# jellyfish mean speed in metres per 10 seconds
parallel lake.speed_dist.scale [expr 0.6*$scale]
# jellyfish mean turn angles (degrees)
parallel lake.angle_dist.scale 24

# discrete distribution of bell sizes (in cms)
# distribution is only used on master processor
unuran radii_dist
radii_dist.set_gen {distr=discr; pv=( 20.62, 20.91, 17.10, 8.98, 6.00, 4.48, 3.28, 2.98, 2.95, 3.08, 3.10, 2.84, 1.93, 1.00, 0.52, 0.18, 0.04, 0.01)}
lake.radii_dist.set_gen radii_dist
# scale distribution to model scale
lake.radii_dist.scale 0.01*$scale

# set the number of cells in x &y direction for the jellyfish neighbourhood map
set nmapx [expr int([lake.width]*$scale) / int([lake.speed_dist.scale]*5)]
set nmapy [expr int([lake.height]*$scale) /int([lake.speed_dist.scale]*5)]
lake.init_map $nmapx $nmapy

# <no. jellies> 
lake.add_jellyfish 1000000

lake.checkpoint bla.ckpt

