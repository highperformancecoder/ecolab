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
	    for {set i 0} {$i<1000} {incr i} {
		generate 100
		set ext [lifetimes]
		if {[llength $ext]} {puts $lifedat $ext}
		condense
	    }
	    mutate
	    puts $logfile "tstep:[tstep] nsp:[density.size] cput:[cputime]"
	    if {[cputime]>10000} {
		checkpoint $checkfname
		close $lifedat
		close $logfile
		exit
	    }

	}
}
}

set jobname "ecohisto[set argv(1)],m=[set argv(2)]"
set checkfname $jobname.ckpt
set lifedat [open life.$jobname.dat a]
fconfigure $lifedat -buffering line
set logfile [open $jobname.log w]
fconfigure $logfile -buffering line

srand [expr abs([clock seconds])]

if {[file exists $checkfname]} {
    restart $checkfname
} else {
set nsp 30
density [constant $nsp 100]

repro_min -.1
repro_max .1
repro_rate [unirand $nsp [repro_max] [repro_min]]

interaction.diag [unirand $nsp -.001 -1e-3]

set conn_per_sp 3
odiag_max [expr 3 *  -[av [interaction.diag]] / $conn_per_sp ]
odiag_min [expr -[odiag_max]]
random_interaction $conn_per_sp 0
interaction.val [unirand [ecolab.interaction.val.size] \
   [ecolab.odiag_min] [ecolab.odiag_max]]

# mutation parameters
mut_max $argv(2)
sp_sep .1
mutation [constant $nsp [ecolab.mut_max]] 
}

simulate

