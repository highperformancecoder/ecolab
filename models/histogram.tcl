#!/bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

# use ecolab interpreter in pwd
#\
    ${0%%histogram.tcl}ecolab $0 $*
#\
    exit
# TCL code begins here
GUI
histogram hist -title $argv(1) {}

namespace eval histo_ns {
    set dat [open $argv(1) r]
    while {![eof $dat]} {
	gets $dat value
        histogram hist $value
    }
}


