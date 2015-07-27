#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


proc dist {x} {return [expr pow($x,2)+pow($x,3)+900]}

distrand PDF
#PDF.min -10
PDF.min 1
PDF.max 10
PDF.nsamp 100
PDF.width 3
PDF.Init dist

proc simulate {} {
    uplevel #0 {
	set running 1
	for {} {$running} { } {
	    histogram testdist [PDF.rand]
	}
    }
}

GUI
simulate
