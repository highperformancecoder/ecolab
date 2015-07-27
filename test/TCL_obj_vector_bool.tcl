#!TCL_obj_vector_bool
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

foo.@elem 3
foo(3) 1
if {[foo(3)]!=1} {exit 1}
foo(3) 0
if {[foo(3)]!=0} {exit 1}
exit 0
