#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

#Tkinit

namespace eval hist {

    set nbins 100
    set xlogison 1

    set dat [open $argv(1) r]
    set data [open hist.histodat w]
    set max -1E38
    set min 1E38
    while {![eof $dat]} {
	gets $dat value
	# value is now potentially a list of values
	for {set j 0} {$j < [llength $value]} {incr j} {
	    scan [lindex $value $j] %f v
	    puts $data $v
	    if {$v > $max} {set max $v}
	    if {$v < $min} {set min $v}
	}
    }
    close $data

    if {$xlogison} {
	set delta [expr (log($max)-log($min))/($nbins-.1)]
	set scale [expr pow(10,-int(log10(log($max)-log($min))))]    
    } else {
	set delta [expr ($max-$min)/($nbins-.1)]
	set scale [expr pow(10,-int(log10($max-$min)))]
    }

    create_vector x $nbins $min $scale
    create_vector y $nbins 0 0
    
    fillyarray hist $min $delta
    outputdat $argv(2) $xlogison $scale x y 
   
}

