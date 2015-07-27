#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc

set maxNodes 20000
set fname 1
while {1} {
    set nodes [expr int([uni.rand]*$maxNodes)]
    if {$nodes<100} {set nodes 100}
#    set links [expr int([uni.rand]*$nodes*($nodes-1))]
    set links [expr int([uni.rand]*10*$nodes)]
    genRandom $nodes $links
    set winner [automorphism_race 600]
    puts stdout "$nodes $links $winner"
    if {$winner==0} {g.output pajek "timedOut/[set fname].net"; incr fname}
    if {$winner==1} {g.output pajek "superNova/[set fname].net"; incr fname}
    #    puts stdout "$nodes $links [automorphism_algorithm_name [automorphism_race 60]]"
}
