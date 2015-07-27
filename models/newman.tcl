#!newman
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


set nsp 10
newman.density [constant $nsp 1]
newman.threshold [unirand $nsp]

affinerand dist
gaussrand gaussian
dist.set_gen gaussian
dist.scale 0.005
dist.offset 0.01
newman.pstress dist

newman.mutation 1e-4

proc simulate {} \
{ uplevel #0 \
  {
    set running 1
    for {} {$running} { } \
	    { 
	for {set i 0} {$i<10} {incr i} {
	    newman.step
	}

	set lt [newman.lifetimes]
#	 if {[llength $lt] > 0} {
#	     puts $lifedat $lt
#	     flush $lifedat
#	 }
#	 puts $nspdat [newman.density.size]
#	 flush $nspdat
	histogram lifetimes $lt
	newman.condense

	set nsp [newman.density.size]
	set tstep [newman.tstep]
	plot nsp  -title "No. of Species" tstep nsp
	.statusbar configure -text "t=$tstep nsp=$nsp"
        update 
      }
  }
}

GUI
#simulate
#exit

