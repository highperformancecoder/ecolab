#!netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
#const_link_search 9 64
for {set link 0} {$link<8*7/2} {incr link} {
    exhaust_search 8 $link
}
set graphno 0
for {first} {![last]} {next} {
    set graph [open $graphno.dot w]
    puts $graph [g]
    close $graph
    puts stdout "$graphno [g.links] [complexity]"
    incr graphno
}

