#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
urand ran
for {set i 0} {$i<1000} {incr i} {
    set nodes [expr int([ran.rand]*4990+10)]
    set links [expr int([ran.rand]*$nodes*($nodes-1)/2)]
    genRandom $nodes $links
    puts stdout "[g.nodes] [g.links] [time complexity]"
}
