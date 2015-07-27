#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


use_namespace ecolab

proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    for {set j 0} {$j<1} {incr j} {
		for {set i 0} {$i<100} {incr i} {
		    ecolab.generate
		}
		ecolab.mutate
	    }
	    connect_plot ecolab.interaction ecolab.density
	    ecolab.condense
	    set nsp [ecolab.density.size]
	    set tstep [ecolab.tstep]
	    set conn [expr double([ecolab.interaction.val.size])/($nsp*$nsp)]
	    plot nsp -title "No. of Species" tstep nsp
	    .statusbar configure -text "t:[ecolab.tstep] nsp:$nsp"
	    nsp::print "nsp$tstep.ps"
	    connect_ecolab_interaction::print "conn$tstep.ps"
	}
    }
}

source model.tcl

set palette {black red green blue magenta cyan yellow}
GUI
