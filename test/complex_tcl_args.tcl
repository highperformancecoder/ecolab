source "assert.tcl"
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace foobar
seta {1 2 3}
assert {[a]=="1 2 3"}
assert {[b] == "foo"}
assert {[testBool 0]==0}
assert {[testBool 2]==1}
assert {[testInt 1234]==1234}
assert {[testUnsigned 1234]==1234}
assert {[testLong 1234]==1234}
assert {[testFloat 1.234]==1.234}
assert {[testDouble 1.234]==1.234}
assert {[testString 1.234]==1.234}
assert {[testEnum "ff"]=="ff"}
assert {[testEnum "bar"]=="bar"}
assert {[testVector {1 2 3 4}]=={1 2 3 4}}
assert {[testMap {1 2 h 3}]=={{1 2} {h 3}}}
