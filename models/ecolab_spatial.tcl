#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


parallel use_namespace ecolab

proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    generate 100
	    mutate
	    condense
	    migrate
	    ecolab.gather
#	    set conn [expr double([interaction.val.size])/($nsp*$nsp)]
#	    plot nsp -title "No. of Species" tstep nsp
#	    plot connsp -title "Connectivity vs diversity"  nsp conn
#	    connect_plot ecolab.interaction ecolab.density
#	    histogram lifetimes [ecolab.lifetimes]
	    set nsp [species.size]
	    display [tstep] ecolab(0,0).density ecolab.species
	    display [tstep] ecolab(0,1).density ecolab.species
	    display [tstep] ecolab(1,0).density ecolab.species
	    display [tstep] ecolab(1,1).density ecolab.species
	    .statusbar configure -text "t:[tstep] nsp:$nsp"
	}
    }
}

parallel source model.tcl

set_grid 2 2

get 0 0
get 0 1
get 1 0
get 1 1
ecolab(0,0).density [constant $nsp 100]
ecolab(0,1).density [constant $nsp 100]
ecolab(1,0).density [constant $nsp 100]
ecolab(1,1).density [constant $nsp 100]

ecolab.distribute_cells
generate
set display_scale 3
set palette {black red green blue magenta cyan yellow}
GUI


