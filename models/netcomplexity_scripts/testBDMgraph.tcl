#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
g.input pajek $argv(1)
set lnomega [lnomega]
set nauty_lnomega [nauty_lnomega]
if {[testAgainstNauty]} {
    puts "$argv(1) PASSED $lnomega $nauty_lnomega"
} else {
    puts "$argv(1) FAILED $lnomega $nauty_lnomega"
    exit 1
}    
