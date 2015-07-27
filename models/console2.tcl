#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


set host localhost
set port 7000
set palette {black red green blue magenta cyan yellow}


proc simulate {} {
    uplevel #0 {
	set channel [socket localhost 7001]
	set s [gets $channel]
	close $channel
	set t [lindex $s 0]
	set n0 [lindex $s 1]
	set n1 [lindex $s 2]
	plot species t n0 n1
	.statusbar configure -text $t
	after 10000 simulate
    }
}


source Xecolab.tcl
