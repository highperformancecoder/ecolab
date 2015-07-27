#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
for {set i 3} {1} {set i [expr $i+2]} {
    genFullDirected $i
    puts stdout "$i [time {set lnomega [lnomega]}]"
#    puts stdout "$i [time {set nauty_lnomega [nauty_lnomega]}]"
#    if {abs($lnomega-$nauty_lnomega)>log(2)} {
#        puts stdout "PASSED $lnomega $nauty_lnomega"
#        exit 1
#    }
}
    
    
