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
	    generate 100
	    mutate
	    connect_plot ecolab.interaction ecolab.density
	    histogram lifetimes [lifetimes]
	    condense
	    set nsp [species.size]
	    set tstep [tstep]
	    set conn [expr double([interaction.val.size])/($nsp*$nsp)]
	    plot nsp -title "No. of Species" tstep nsp
	    plot connsp -title "Connectivity vs diversity"  nsp conn
            set dd [density]
	    display -title "Population density" \
		       [tstep] ecolab.density ecolab.species
            if {[tstep] % 1000 == 0} {
                foodweb.output pajek foodweb[format "%08d" [tstep]].net
            }
#            plot complexity tstep [complexity]
	    .statusbar configure -text "t:[tstep] nsp:$nsp"
	}
    }
}

source model.tcl

set display_scale 3
set palette {black red green blue magenta cyan yellow}
GUI
#.user1 configure -command {ecolab.random_interaction}
#simulate
