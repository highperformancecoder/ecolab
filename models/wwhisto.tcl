use_namespace webworld
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


set t1 0
set extsum 0
proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} {incr t1} {
	    for {set t 0} {$t<5} {incr t} {
		generate 0.2
	    }
	    set ext [lifetimes]
	    if {[llength $ext]} {puts $lifedat $ext; flush $lifedat}
	    set extsum [expr $extsum + [llength $ext]]
	    condense
	    if {$t1%20==0} {
		mutate
		if {$extsum} {puts $extdat $extsum; flush $extdat}
		set extsum 0
	    }
	    if {[cputime]>10000} {
		checkpoint $checkfname
		close $lifedat
		close $extdat
		exit
	    }
	}
}
}

set jobname "wwhisto[set argv(1)],c=[set argv(2)]"
set checkfname $jobname.ckpt
set lifedat [open life.$jobname.dat a]
set extdat  [open ext.$jobname.dat a]

webworld.unirand.seed [expr abs([clock seconds])]

if {[file exists $checkfname]} {
    restart $checkfname
} else {
    R 1E5
    b 5E-3
    c $argv(2)
    niter 1
    lambda 0.1
    N [constant 15 100]  
    initM 500  # no. of possible features in a pool
    init_interaction 10 # no.features in a genotype
}

simulate

