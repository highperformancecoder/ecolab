#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc

set maxNodes 100
while {1} {
    set nodes [expr int([uni.rand]*$maxNodes)]
    if {$nodes<10} {set nodes 10}
    set links [expr int([uni.rand]*$nodes*($nodes-1))]
    genRandom $nodes $links
    set time [time {set w [bliss_race 60]}]
    puts stdout "$nodes $links $w $time"
}
