#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

GUI
set K 10
gaussrand g
set n $K
set D 100

set t 0
set t1 0

proc simulate {} {
    uplevel #0 {
    set running 1
    while {$running} {
#	set n [g.rand]
	histogram dist $D
#	histogram dist $n
	set n [expr (1+exp($D*1e-2*[g.rand])-0.01*($n-$K))*$n]
#	set n [expr (exp($D*1e-2*[g.rand]))*$n]
	incr t
	incr t1
	if {$t%10==0} {incr D}
#	plot div t D
	if {$n<1} {
	    histogram life $t1
	    incr D -1
	    set n $K
	    set t1 0
	}
    }
}
}
