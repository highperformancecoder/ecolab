#!/bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

#PBS -A rks -q checkable -lnodes=2:ppn=2 -lwalltime=48:0:0 
#\
ulimit -c unlimited
#\
cd $PBS_O_WORKDIR
#\
exec time lrun jellyfish jellyfish-batch.tcl 

set startclock [clock seconds]
puts stdout "Started at [clock format $startclock]"

source lakes/OTM
parallel use_namespace lake
parallel use_namespace jfish_parms

set time 6
#set time 7
set startdate 1
set day $startdate
# month 0 = January
set month 10
set latitude 7

parallel lake.restart [file rootname $lakef].bin
parallel mangrove_height [lake.mangrove_height]
parallel uni.seed {[expr 100*([myid]+1)]}
parallel gaussrand gran
parallel gran.uni.seed {[expr 100*([myid]+1)]}
parallel speed_dist.set_gen gran
parallel angle_dist.set_gen gran
parallel photo_taxis .1
# turning probability
parallel change_prob .8


proc set_tstep {newval} {
    global tstep scale
    #timestep in seconds
    set tstep $newval
    
    # jellyfish mean speed in metres per timestep
    parallel speed_dist.scale [expr 0.06*$scale*$tstep]
    parallel max_speed [expr 3*[speed_dist.scale]]
    parallel vz_dist.scale [expr 0.005*$scale*$tstep]

# set the number of cells in each direction for the jellyfish neighbourhood map
    parallel nmapx [expr int([width] / ([speed_dist.scale]*7))]
    parallel nmapy [expr int([height] /([speed_dist.scale]*7))]
#    parallel nmapz [expr -int([depth] /([vz_dist.scale]*5))]
    parallel nmapz 1
    puts stdout "[nmapx] [nmapy] [nmapz]"
    puts stdout "be init_map"
    init_map       
    puts stdout "after init_map"


}

#init timestep in seconds
set_tstep 10

# jellyfish mean turn angles (degrees)
parallel angle_dist.scale 24

# 3D model parameters
parallel depth [expr -5*$scale]
parallel ideal_depth [expr -4*$scale]
parallel vert_change_prob 0.2
parallel vz_dist.set_gen gran

# discrete distribution of bell sizes (in cms)
# distribution is only used on master processor
unuran rdist
rdist.set_gen {distr=discr; pv=(0.0, 20.62, 20.91, 17.10, 8.98, 6.00, 4.48, 3.28, 2.98, 2.95, 3.08, 3.10, 2.84, 1.93, 1.00, 0.52, 0.18, 0.04, 0.01)}
radii_dist.set_gen rdist
# scale distribution to model scale
radii_dist.scale 0.01*$scale

puts stdout "cputime=[cputime]"
# <no. jellies> 
#add_jellyfish 13500000
add_jellyfish 5000

puts stdout "cputime=[cputime]"
set output [open "jelly-batch.out" w]

#clear_non_local

set upd_ctr 0
Tkinit
image create photo density -height [height] -width [width]
contrast 100
proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$time <12} { } { 
#	    lake.gather
#	    puts $output "[time $time] [date [dayofyear $day $month]] msize=[jelly_map.max_size] [cputime]secs [expr [clock seconds]-$startclock] wall secs"
	    puts $output "[time $time] [date [dayofyear $day $month]] [cputime]secs [expr [clock seconds]-$startclock] wall secs"
	    puts stdout "[time $time] [date [dayofyear $day $month]] [cputime]secs [expr [clock seconds]-$startclock] wall secs"
	    flush $output
	    set T [expr int($time)]
	    if {![info exists ckwritten($T)]} {
		lake.gather
		lake.checkpoint lake$T.ckpt
		set ckwritten($T) true
	    } 
	    if {abs($time-7.5)<=1.5 || abs($time-17)<=1} {
		# update shadow lines every 10 seconds
		set shad_update_freq [expr 10/$tstep]
	    } else {
		# update shadow lines every 6 minutes
		set shad_update_freq [expr 360 / $tstep]
	    }
	    for {set i 0} {$i<$shad_update_freq} {incr i} {
		lake.update
		incr upd_ctr
		if {$upd_ctr%10==0} {
                    puts stdout "b4 update_map"
		    lake.update_map
                    draw_density density
                    density write "density[time $time].ppm" -format ppm
		}
	    }
	    set time [expr $time+$shad_update_freq*$tstep/3600.0]
	    if {$time>=24} {
		set time [expr fmod($time,24)]
		incr day
	    }
	    eval lake.compute_shadow [sun_angle]
# This idea of reducing timestep didn't seem to work too well.
#	    if {[lake.cell_max]>5000} {
#		set_tstep [expr $tstep * 0.8]
#		update_map
#		puts stdout "tstep->$tstep"
#	    }
	}
    }   
}

# date = day in year
# time = hours (24 hour clock)
proc sun_angle {} {
    global latitude time day month
    set date [dayofyear $day $month]
    set pi2 6.2831853
    set theta [expr ($time/24.0+.5)*$pi2]
    set phi [expr sin(($date-110)/365.0*$pi2)*23.0/360.0*$pi2]
    set alpha [expr $latitude/360.0*$pi2]
    return [list [expr -cos($phi)*sin($theta)] \
	 [expr -cos($phi)*cos($theta)*sin($alpha)+sin($phi)*cos($alpha)]\
	 [expr cos($phi)*cos($theta)*cos($alpha)+sin($phi)*sin($alpha)]\
    ]
}


proc time {t} {return [expr int($t)]:[format "%02d" [expr int(fmod($t*60,60))]]}

eval lake.compute_shadow [sun_angle]

simulate
puts stdout "ncpus=[nprocs] cputime=[cputime]"
exit
