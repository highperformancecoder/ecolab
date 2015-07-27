#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
urand ran
set maxNodes 500
for {set i 0} {$i<1000} {incr i} {
    set nodes [expr int([uni.rand]*$maxNodes)]
    if {$nodes<100} {set nodes 100}
    set links [expr int([uni.rand]*10*$nodes)]
    genRandom $nodes $links
    g.output pajek randGsamples/rand$i.net
}
