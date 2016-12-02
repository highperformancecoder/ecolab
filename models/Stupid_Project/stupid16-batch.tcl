#!stupid16
use_namespace stupidModel

proc simulate {} { 
    uplevel #0 {
      set running 1
         for {} {$running} { } {
	     grow
	     moveBugs
	     birthdeath
	     hunt
	     if {[tstep]==1000 || [bugs.size]==0} {set running 0}
	 }
    }
}

u.seed 100

set mv_dist 4
read_food_production Stupid_Cell.Data $mv_dist

set nbugs 100
bugArch.max_consumption 1
bugArch.size 1
bugArch.repro_size 10
bugArch.survivalProbability 0.95

# initials bug sizes are drawn from a distribution
gaussrand normal
affinerand initDist
initDist.set_gen normal
initDist.scale 0.03
initDist.offset 1.0
initBugDist initDist

addBugs $nbugs 

addPredators 200

#new ones created afterwards are size 0
bugArch.size 0

# size of a grid cell in pixels
set scale 5
stupidModel.scale 5 

simulate
