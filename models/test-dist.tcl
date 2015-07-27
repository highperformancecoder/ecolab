#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


unuran rand
rand.set_gen {distr=discr; pv=(0.5,0.2,0.3)}
rand.set_gen {distr=discr; pv=( 20.62, 20.91, 17.10, 8.98, 6.00, 4.48, 3.28, 2.98, 2.95, 3.08, 3.10, 2.84, 1.93, 1.00, 0.52, 0.18, 0.04, 0.01)}
rand.set_gen "normal; domain=(0,inf)"

proc simulate {} { 
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    set r [rand.rand]
#	    puts stdout $r
	    histogram dist [expr $r+.5]
	}   
    }   
}


GUI
