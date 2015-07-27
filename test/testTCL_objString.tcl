#!testTCL_objString
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


source assert.tcl

use_namespace foo
bar "hello world"
assert {[bar]=="hello world"}
