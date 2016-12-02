#!stupid15
use_namespace stupidModel

proc simulate {} { 
    uplevel #0 {
      set running 1
         for {} {$running} { } {
	    grow
	    moveBugs
	    birthdeath
	    drawCells .world.canvas
	    drawBugs .world.canvas
	     set maxsize [max [bugsizes]]
	    .statusbar configure -text "t:[tstep] max size:$maxsize"
	     set t [tstep]
	     plot ms -title "Max size" t [max_bugsize]
	     plot nb -title "No. bugs" t [bugs.size]
	     if {[tstep]==1000 || [bugs.size]==0} {set running 0}
	 }
    }
}

set mv_dist 4
read_food_production Stupid_Cell.Data $mv_dist

set nbugs 100
bugArch.max_consumption 1
bugArch.size 1
bugArch.repro_size 10
bugArch.survivalProbability 0.9

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
canvas .world.canvas -background black -width [expr [nx]*$scale] \
    -height [expr [ny]*$scale]
pack .world.canvas

drawCells .world.canvas
drawBugs .world.canvas

bind .world.canvas <Button-1> {obj_browser [probe %x %y cell].*}
bind .world.canvas <Button-3> {obj_browser [probe %x %y bug].*}




