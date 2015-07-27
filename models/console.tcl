#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


set host localhost
set port 7000
set palette {black red green blue magenta cyan yellow}

proc simulate {} {
    uplevel #0 {
        if {!$running} return
	ecolab.get_vars $host $port
	after 1000 simulate
	if {[ecolab.density.size]>0} {
	    display  -title "Population density" \
		    [ecolab.tstep] ecolab.density ecolab.species
	    .statusbar configure -text "tstep: [ecolab.tstep]"
	}
    }
}


source Xecolab.tcl
