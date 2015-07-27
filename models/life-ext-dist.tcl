#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

# use as life-ext-dist.tcl lifetimes.dat
GUI
histogram lifehist  -title $argv(1) {}

# load up lifetimes histogram
set dat [open $argv(1) r]
set data [open lifehist.histodat w]
while {![eof $dat]} {
    gets $dat value
    # place data directly in lifetime histogram file
    for {set j 0} {$j < [llength $value]} {incr j} {
	set v [lindex $value $j]
	puts $data $v
	if {$v > $lifehist::max} {set lifehist::max $v}
	if {$v < $lifehist::min} {set lifehist::min $v}
    }
}
close $dat
close $data
set lifehist::reread 1
histogram lifehist {}

histogram exthist -title $argv(1) {}

set sumperiod 8  
scale .exthist.sumperiod -from 0 -to 32 -showvalue false \
    -length [expr [.exthist.graph cget -height]-7] \
    -variable sumperiod -command sumperlabel
pack append .exthist .exthist.sumperiod right

proc sumperlabel {val} {
	.exthist.sumperiod configure -label [expr 1<<$val]
}    

# a clear routine - merge this into histogram code!
namespace eval exthist {
    proc clear {} {
	variable data 
	variable max 
	variable min
	variable reread
	close $data
	set data [open exthist.histodat w]
	set max -1E38
	set min 1E38
	set reread 1
	histogram exthist {}
    }
}
 

# calculate extinction avalanche sizes
proc simulate {} {
    global sumperiod argv
    set dat [open $argv(1) r]
    set ctr 0
    set extsum 0
    exthist::clear
    while {![eof $dat]} {
	gets $dat value
	incr extsum [llength $value]
	incr ctr
	if {$ctr==1<<$sumperiod} {
	    histogram exthist $extsum
	    set ctr 0
	    set extsum 0
	}
    }
    .run configure -relief raised
    close $dat
}

simulate
