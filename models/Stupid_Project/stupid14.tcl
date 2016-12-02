#!stupid14
use_namespace stupidModel

proc simulate {} { 
    uplevel #0 {
      set running 1
         for {} {$running} { } {
	    grow
	    moveBugs
	    birthdeath
	    draw .world.canvas
	     set maxsize [max [bugsizes]]
	    .statusbar configure -text "t:[tstep] max size:$maxsize"
	     set t [tstep]
	     plot ms -title "Max size" t [max_bugsize]
	     plot nb -title "No. bugs" t [bugs.size]
	     if {[tstep]==1000 || [bugs.size]==0} {set running 0}
	 }
    }
}

set toroidal 1
set nx 100
set ny 100
set mv_dist 4
set max_food_production 0.1
setup $nx $ny $mv_dist $toroidal $max_food_production

set nbugs 100
bugArch.max_consumption 1
bugArch.size 1
bugArch.repro_size 10
bugArch.survivalProbability 0.995

# initials bug sizes are drawn from a distribution
gaussrand normal
affinerand initDist
initDist.set_gen normal
initDist.scale 0.03
initDist.offset 1.0
initBugDist initDist

addBugs $nbugs 

#new ones created afterwards are size 0
bugArch.size 0

# size of a grid cell in pixels
set scale 5
stupidModel.scale 5 

GUI
toplevel .world
canvas .world.canvas -background black -width [expr $nx*$scale] \
    -height [expr $ny*$scale]
pack .world.canvas

draw .world.canvas

bind .world.canvas <Button-1> {obj_browser [probe %x %y cell].*}
bind .world.canvas <Button-3> {obj_browser [probe %x %y bug].*}




