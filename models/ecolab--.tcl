#!ecolab--
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace ecolab

r 0.1
beta 1e-3
gaussrand gran
#gran.uni.seed [expr abs([clock seconds])]
gamma.set_gen gran
gamma.scale 0.4

GUI

n 10
tstep 0
while 1 {
    generate 100
    if {[n] ==0 } {
	histogram lifetimes [tstep]
	n 10
	tstep 0
    }
#    .statusbar configure -text "t:[tstep] n:[n]"
    
}


    
