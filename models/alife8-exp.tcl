#!shadow
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

source "model.tcl"
set newact_thresh 50

proc simulate {} { 
    uplevel #0 {
	set running 1
	#       for {} {$running} { } { #}
	    while {[ecolab.tstep] < 1e8} {    
	    for {set i 0} {$i<10} {incr i} {
		ecolab.generate 100
		ecolab.condense
		ecolab.mutate
	    }

	    set nsp [ecolab.density.size]
	    set activity [av [ecolab.activity]]
	    set new_activity [ecolab.newact]
	    set tstep [ecolab.tstep]
	    set conn [expr double([ecolab.interaction.val.size])/($nsp*$nsp)]
	    puts stdout "$tstep $nsp $activity $new_activity $conn" 
	    flush stdout
	}
    }
}

simulate
