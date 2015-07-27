#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
g.input pajek $argv(1)
set nodes [g.nodes]
#set nodes 1000
#set links [g.links]
#puts stdout "$nodes nodes, $links links"

#unuran wdist
#wdist.set_gen "normal; domain=(0,inf)" 
#unuran degree_dist
##degree_dist.set_gen "beta(2,1)"
#degree_dist.set_gen "exponential(3)"
##affinerand degree_dist
##degree_dist.set_gen one
##degree_dist.scale 10
#prefAttach $nodes degree_dist

#GUI
#update_degrees
#histogram outdegree [netc.degrees.out_degree]
#histogram indegree [netc.degrees.in_degree]
#genRandom $nodes $links

puts -nonewline stdout "$argv(1) [g.nodes] [g.links] "
set complexity [complexity]
set fmt "%3.1f"
puts -nonewline stdout "[format $fmt $complexity] "
flush stdout
#HistoStats ws
#ws.add_data [weights]
#set wdestdata [string map {" " ", "} [ws.histogram]]
#unuran wdist
#puts stdout "weight dist=discr; pv=($wdestdata)"
#wdist.set_gen "discr; pv=($wdestdata)" 
#weight_dist wdist
##weight_dist.set_gen "normal; domain=(0,inf)"
#update_degrees
#HistoStats degreeHist
#degreeHist.add_data [degrees.in_degree]
#unuran degree_dist
#set degdestdata [string map {" " ", "} [degreeHist.histogram]]
#puts stdout "degree dist=discr; pv=($degdestdata)"
#degree_dist.set_gen "discr; pv=($degdestdata)" 

HistoStats Rewire
for {set i 1} {$i<100} {incr i} {
    random_rewire
    Rewire.add_data [expr log([complexity])]
}

#GUI
#histogram h [Rewire]
puts -nonewline stdout "[format $fmt [expr exp([Rewire.av])]] [format $fmt [expr $complexity-exp([Rewire.av])]] "
flush stdout
if {[Rewire.stddev] > 0} {
    puts stdout "[format $fmt [expr abs(log($complexity)-[Rewire.av])/[Rewire.stddev]]]"
} else {puts stdout "infinity"}

