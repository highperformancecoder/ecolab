#!accessor
use_namespace foo
source "assert.tcl"

assert {[x hello]=="hello"}
assert {[m_x]=="hello"}
assert {[x]=="hello"}
exit
