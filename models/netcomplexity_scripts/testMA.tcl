#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
urand ran
ran.seed [clock seconds]
for {set i 0} {$i<1000} {incr i} {
    set nodes [expr int([ran.rand]*500+10)]
#    set links [expr int([ran.rand]*$nodes*($nodes-1)/2)]
#    set links [expr int([ran.rand]*2*$nodes)]
    set links [expr $nodes*($nodes-1)/2-int([ran.rand]*2*$nodes)]
    genRandom $nodes $links
    puts stdout "[complexity] [MA]"
#    puts stdout "C: [time {complexity}] MA: [time {MA}]" 
}

