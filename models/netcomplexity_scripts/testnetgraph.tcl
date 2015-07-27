#!netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc

GUI
exhaust_search 6
first
proc simulate {} {
    netgraph net [currgraph]
    next
    if {[last]} {first}
}
