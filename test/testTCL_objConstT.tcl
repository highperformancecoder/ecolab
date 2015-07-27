#!testTCL_objConstT
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


source assert.tcl

assert {[foo.bar]=="hello"}
assert {[foo.staticBar]=="hello"}
if {[catch {foo.bar "barf"}] && [catch {foo.staticBar "barf"}]} {} else {
    puts stderr "Expected exception not thrown"
    exit 1
}

foo.f
if [catch cfoo.f] {} else {
    puts stderr "Expected exception not thrown"
    exit 1
} 
