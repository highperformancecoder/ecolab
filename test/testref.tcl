#!testref
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


foo.a.x 1
foo.b.x 2
if {[foo.a.x]!=1 || [foo.b.x]!=2} {exit 1}
foo.asg_atob
if {[foo.a.x]!=1 || [foo.b.x]!=1} {exit 1}

