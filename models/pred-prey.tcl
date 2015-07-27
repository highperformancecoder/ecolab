#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


#ecolab.density {100 100} 
#ecolab.repro_rate {.1 -.1}
#ecolab.interaction.diag {-.0001 0}
#ecolab.interaction.val {-0.00105 0.00105}
#ecolab.interaction.row {0 1}
#ecolab.interaction.col {1 0}
ecolab.species {1 2}
ecolab.density {100 100} 
ecolab.create {0 0}
ecolab.repro_rate {.1 -.1}
ecolab.interaction.diag {-.0001 -1e-5}
ecolab.interaction.val {-0.001 0.001}
ecolab.interaction.row {0 1}
ecolab.interaction.col {1 0}
ecolab.mutation {.01 .01}
ecolab.sp_sep .1
ecolab.repro_min -.1
ecolab.repro_max .1
ecolab.odiag_min -1e-3
ecolab.odiag_max 1e-3
ecolab.mut_max .01

set palette {black red}

proc simulate {} \
{ uplevel #0 \
	    {
	set running 1
	for {} {$running} { } \
		{ 
	    ecolab.generate
	    .statusbar configure -text "t=[ecolab.tstep] n=[ecolab.density]"
	    set n0 [lindex [ecolab.density] 0]
	    set n1 [lindex [ecolab.density] 1]
	    set tstep [ecolab.tstep]
	    plot density tstep n0 n1
	}
    }
}

source $ecolab_library/Xecolab.tcl


