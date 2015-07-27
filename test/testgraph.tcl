#!testgraph
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

proc assert {x args} {
    if {![expr $x]}  {
        puts stderr "assertion: $x failed: $args"
        exit 1
    }
}

use_namespace tg
first 4 4
assert {[g] == "digraph {\n0->1;\n0->2;\n0->3;\n1->0;\n}\n"}
assert {[dg]== "graph {\n0--1;\n0--2;\n0--3;\n1--2;\n}\n"}

# test assignment
DiGraph ng
ng [g]
assert {[g]==[ng]} "dot asg"
ng.delete

DiGraph ng
ng tg.g
assert {[g]==[ng]} "inbuilt asg"
ng.delete

BiDirectionalGraph ndg
ndg [dg]
assert {[dg]==[ndg]} "bidi dot asg"
ndg.delete

BiDirectionalGraph ndg
ndg tg.dg
assert {[dg]==[ndg]} "bidi inbuilt asg"
ndg.delete

DiGraph ng
BiDirectionalGraph ndg
ng tg.dg
ndg ng
assert {[ndg]==[dg]} "inbuilt tmp asg"
ng ndg
ndg ng
assert {[ndg]==[dg]}
ng.delete
ndg.delete

g.output pajek g.net
g.output lgl g.lgl
dg.output pajek dg.net
dg.output lgl dg.lgl

DiGraph ng
ng.input pajek g.net
assert {[ng]==[g]} "pajek input"
ng.delete

# LGL loses directionality, so we must symmetrise g
DiGraph ng 
DiGraph ng2 
BiDirectionalGraph ndg
ng.input lgl g.lgl
# symmetrise g
ndg tg.g
ng2 ndg
assert {[ng]==[ng2]} "lgl input"
ng.delete
ng2.delete
ndg.delete

BiDirectionalGraph ng
ng.input pajek dg.net
assert {[ng]==[dg]} "pajek bidi inpout"
ng.delete

BiDirectionalGraph ng
ng.input lgl dg.lgl
assert {[ng]==[dg]} "lgl bidi input"
ng.delete

# by generating a fully connected graph, we ensure that node
# relabelling in the LGL routines is uniquely determined.
randGraph 4 12
dg tg.g
BiDirectionalGraph ng
foreach fmt {pajek lgl dot} {
    dg.output $fmt g.net
    ng.Clear
    ng.input $fmt g.net
    assert {[dg]==[ng]} $fmt
}
ng.delete

DiIGraph ig
first 4 4
ig tg.g
puts stdout [ig]
puts stdout [tg.g]

assert {[ig]==[tg.g]} "ig load"
ig.delete
