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
	     drawCells .world.canvas
	     set maxsize [max_bugsize]
	    .statusbar configure -text "t:[tstep] max size:$maxsize"
	     set t [tstep]
#	     plot ms -title "Max size" t maxsize
#	     plot nb -title "No. bugs" t [bugs.size]
	     if {[tstep]==1000 || [max_bugsize]<0} {set running 0}
	 }
    }
}

u.seed [clock seconds]

set mv_dist 4
#setup 100 100 4 0 0.01
read_food_production Stupid_Cell.Data $mv_dist

set nbugs 100
bugArch.max_consumption 1
bugArch.size 1
bugArch.repro_size 10
bugArch.survivalProbability 0.99


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

GUI
toplevel .world
canvas .world.canvas -background black -width [expr [nx]*$scale] \
    -height [expr [ny]*$scale]
pack .world.canvas
image create photo food -width [expr [nx]*$scale] \
    -height [expr [ny]*$scale]
.world.canvas create image 0 0 -anchor nw -image food


drawCells .world.canvas

bind .world.canvas <Button-1> {obj_browser [probe %x %y cell].*}
bind .world.canvas <Button-3> {obj_browser [probe %x %y bug].*}

set filetypes {{{Checkpoint files} .ckpt} {{All files} *}}

.user1 configure -text "Checkpoint" -command {
    checkpoint [tk_getSaveFile -filetypes $filetypes -title "Checkpoint filename"]}

.user2 configure -text "Restart" -command {
    restart [tk_getOpenFile -filetypes $filetypes -title "Checkpoint filename"]
    drawCells .world.canvas
    .statusbar configure -text "t:[tstep] max size:[max_bugsize]"
}


