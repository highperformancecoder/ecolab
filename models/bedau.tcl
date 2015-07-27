#!shadow
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


# initial condition
set nsp 100
ecolab.density [constant $nsp 100]

ecolab.repro_min -.1
ecolab.repro_max .1
ecolab.repro_rate [unirand $nsp [ecolab.repro_max] [ecolab.repro_min]]

ecolab.interaction.diag [unirand $nsp -.0001 -1e-5]

ecolab.odiag_min -1e-4
ecolab.odiag_max 1e-4
ecolab.random_interaction 3 0
ecolab.interaction.val [unirand [ecolab.interaction.val.size] \
   [ecolab.odiag_min] [ecolab.odiag_max]]

# mutation parameters
ecolab.mut_max 1e-4
ecolab.sp_sep .1
ecolab.mutation [constant $nsp [ecolab.mut_max]] 

set newact_thresh 10

proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    for {set j 0} {$j<10} {incr j} {
		for {set i 0} {$i<100} {incr i} {
		    ecolab.generate
		}
		ecolab.condense
		ecolab.mutate
	    }
#	    connect_plot ecolab.interaction ecolab.density
	    set nsp [ecolab.density.size]
	    set activity [av [ecolab.activity]]
	    set new_activity [ecolab.newact]
	    set tstep [ecolab.tstep]
#	    set conn [expr double([ecolab.interaction.val.size])/($nsp*$nsp)]
	    plot nsp -title "Diversity" tstep nsp
	    plot act -title "Mean Cumulative Activity" tstep activity
	    plot new -title "New Activity" tstep new_activity
#	    plot connsp -title "Connectivity vs diversity"  nsp conn
	    display -title "Population density" \
		    [ecolab.tstep] ecolab.density ecolab.species
	    .statusbar configure -text "t:[ecolab.tstep] nsp:$nsp"
	}
    }
}


set display_scale 3
set palette {black red green blue magenta cyan yellow}

GUI


