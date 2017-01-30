#!test_tcl_stl
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

#GUI

vec.@elem 2
vec(2) 3
if {![check_vec 2 3]} {exit 1}

if {[vec.size]!=10} {exit 1}

vec {1 3 5 7}
if {![check_vec 2 5]} {exit 1}
set l [vec]
if {[lindex $l 2]!=5} {exit 1}

deq.@elem 2
deq(2) 3
if {![check_deq 2 3]} {exit 2}

deq {1 3 5 7}
if {![check_deq 2 5]} {exit 1}
set l [deq]
if {[lindex $l 2]!=5} {exit 1}

l.@elem 2
l(2) 3
if {![check_list 2 3]} {exit 3}

l {1 3 5 7}
if {![check_list 2 5]} {exit 1}
set l [l]
if {[lindex $l 2]!=5} {exit 1}

# for some reason this command no longer works under TCL_eval file
#if {[s.#members]!="1 3 5"} {exit 4}

s.@elem 1
if {[s(1)]!=3} {exit 5}

s {1 3 5 7}
if {![check_set 2 5]} {exit 1}
set l [s]
if {[lindex $l 2]!=5} {exit 1}


m1.@elem 1
m1.@elem 5
m1(1) 3
m1(5) 4
if {[m1(1)]!=3 && [m1(5)]!=4} {exit 6}

m2.@elem house
m2.@elem mouse
m2(house) 3
m2(mouse) 4
if {[m2(house)]!=3 && [m2(mouse)]!=4} {exit 7}

string1 hello
if {[string1]!="hello"} {exit 8}

string2 hello
if {[string2]!="hello"} {exit 9}

vstring.@elem 0
vstring(0) hello
if {[vstring(0)]!="hello"} {exit 10}

vvstring.@elem 0
vvstring(0) {hello world}
if {[vvstring]!="{hello world} "} {exit 11}

# the following lines should trigger a bug when same chaperone passed
# to multiple commands
MII mii
mii.delete

VI vi
vi.delete
