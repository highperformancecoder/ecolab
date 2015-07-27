#!/bin/bash
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


submit ()
{
  while [ `jobs|wc -l` -gt 4 ]; do sleep 1; done 
  $* & 
}

ulimit -t 600
for graph in randGsamples/*.net; do
  echo $graph
#  submit time_saucy.tcl $graph >$graph.saucy.log
  submit testBDMgraph.tcl $graph >>testGraph.log
#  submit time_nauty.tcl  $graph >$graph.nauty.log
done
