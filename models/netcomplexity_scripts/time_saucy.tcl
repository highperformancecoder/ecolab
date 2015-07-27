#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
g.input pajek $argv(1)
proc saucy {fname} {
   exec ~/saucy-1.99.6/saucy [set fname]
}
puts stdout "[g.nodes] [g.links] [time {saucy [file rootname $argv(1)].saucy}]"
