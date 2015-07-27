#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
foreach fname [glob  randGsamples/*] {
    if {[file size $fname]==0} {
        g.input pajek [file rootname [file rootname $fname]]
        set output [open $fname w]
        puts $output "[g.nodes] [g.links] 6E8"
        close $output
    }
}

