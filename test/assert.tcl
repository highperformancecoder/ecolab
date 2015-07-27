
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

proc assert {x args} {
    upvar _assertArg_ _assertArg_
    set _assertArg_ $x
    upvar _assertArgs_ _assertArgs_
    set _assertArgs_ $args
    uplevel 1 {
        if {![expr $_assertArg_]}  {
            puts stderr "assertion: $_assertArg_ failed: $_assertArgs_"
            exit 1
        }
    }
}
