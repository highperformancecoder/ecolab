#!../models/ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


GUI

toplevel .plotImage
image create photo .plotImage.image -width 500 -height 500
label .plotImage.label -image .plotImage.image
pack .plotImage.label

Plot testPlot
testPlot.image .plotImage.image 0
#testPlot.plotType bar
puts "plotType=[testPlot.plotType]"
testPlot.grid 1
testPlot.leadingMarker 1
testPlot.nxTicks 3
testPlot.nyTicks 3
testPlot.legend 1
testPlot.legendSide right  
testPlot.labelPen 0 nought
testPlot.labelPen 1 one
testPlot.labelPen 2 two
testPlot.labelPen 3 three
testPlot.labelPen 4 four
testPlot.labelPen 5 five
testPlot.labelPen 6 six
testPlot.labelPen 7 seven

testPlot.assignSide 2 right

testPlot.xlabel xlabel
testPlot.ylabel ylabel
testPlot.y1label y1label


urand r

for {set i 0} {$i<10} {incr i} {
    testPlot.plot $i "[r.rand] [r.rand] [expr 10*[r.rand]] [r.rand] [r.rand] [r.rand] [r.rand] [r.rand]" 
}
gets stdin

proc doPlot {nPens nx} {
    testPlot.clear
    for {set i 0} {$i<$nx} {incr i} {
        set d ""
        for {set p 0} {$p<$nPens} {incr p} {lappend d [expr [r.rand]/[r.rand]]}
        testPlot.plot [expr $i+90] $d
    }
    testPlot.redraw
    gets stdin
}

for {set nx 2} {$nx<16} {incr nx $nx} {
    for {set nPens 1} {$nPens<8} {incr nPens} {
        puts "nPens=$nPens nx=$nx"
        testPlot.legendSide left
        doPlot $nPens $nx
        testPlot.legendSide right
        doPlot $nPens $nx
    }
}

testPlot.clear
testPlot.plotType 0
for {set i 0} {$i<10} {incr i} {
    testPlot.plot $i 100
}
gets stdin

exit
