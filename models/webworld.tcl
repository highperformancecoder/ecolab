#!webworld
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


use_namespace webworld

set t1 0
set extsum 0
proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    for {set t 0} {$t<5} {incr t} {
		generate 0.2
		.statusbar configure -text "t:[t] nsp:[expr [N.size]-1]"
	    }
#	    histogram lifetimes [lifetimes]
	    set extsum [expr $extsum + [condense]]
	    set D [N.size]
	    set C [expr [S.val.size] / double($D*$D)]
	    plot conndiv -title "Connectivity-Diversity" D C
	    plot diversity [t] D
	    plot dissipation [t] [dissipation]
	    plot complexity [t] [complexity]
	    display -title "Population" [t] webworld.N webworld.species
            if {[t] % 100 == 0} {
                foodweb.output pajek webworld_foodweb[format "%08d" [t]].net
            }
            plot complexity [t] [complexity]
	    incr t1
	    if {$t1%20==0} {
		mutate
#		if {$extsum} {histogram ext -title "Extinction Avalanches" $extsum}
		set extsum 0
	    }

	}
}
}

R 1E5
b 5E-3
c 0.8
lambda 0.1
N [constant 15 100]  
initM 500  # no. of possible features in a pool
init_interaction 10 # no.features in a genotype


set display_scale 3
set palette {black red green blue magenta cyan yellow}
#GUI
simulate

