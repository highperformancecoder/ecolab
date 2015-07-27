#!testCairo
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


GUI
createTestCairo

toplevel .test
image create photo .test.image -width 250 -height 250
#testCairo .test.image

label .test.label -image .test.image
#pack .test.label

canvas .test.canvas -width 500 -height 500
pack .test.canvas

proc moveItem {id x y} {
    set coords [.test.canvas coords $id]
    .test.canvas move $id [expr $x-[lindex $coords 0]] [expr $y-[lindex $coords 1]]
}


set id [.test.canvas create testCairo 250 250  -image .test.image -scale 1 -rotation 0]
set id [.test.canvas create testCairo 250 250  -scale 1 -rotation 0]
.test.canvas bind $id <B1-Motion> "moveItem $id %x %y"

exit
