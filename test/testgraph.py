#!python3
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

from testgraph import tg
from ecolab import DiGraph, BiDirectionalGraph

tg.first(4, 4)
assert tg.g.dot() == "digraph {\n0->1;\n0->2;\n0->3;\n1->0;\n}\n"
assert tg.dg.dot()== "graph {\n0--1;\n0--2;\n0--3;\n1--2;\n}\n"

# test assignment
ng=DiGraph()
ng.importDot(tg.g.dot())
assert tg.g.dot()==ng.dot(), "dot asg"

ndg=BiDirectionalGraph()
ndg.importDot(tg.dg.dot())
assert tg.dg.dot()==ndg.dot(), "bidi dot asg"

ng=DiGraph()
ndg=BiDirectionalGraph()
ng.importDot(tg.dg.dot())
ndg.importDot(tg.dg.dot())
assert ndg.dot()==tg.dg.dot(), "inbuilt tmp asg"

tg.g.output("pajek","g.net")
tg.g.output("lgl","g.lgl")
tg.dg.output("pajek","dg.net")
tg.dg.output("lgl","dg.lgl")

ng=DiGraph()
ng.input("pajek","g.net")
assert ng.dot()==tg.g.dot(), "pajek input"

# LGL loses directionality, so we must symmetrise g
ng=DiGraph()
ng2=DiGraph()
ndg=BiDirectionalGraph()
ng.input("lgl", "g.lgl")
# symmetrise g
ndg.importDot(tg.g.dot())
ng2.importDot(ndg.dot())
assert ng.dot()==ng2.dot(), "lgl input"

ng=BiDirectionalGraph()
ng.input("pajek","dg.net")
assert ng.dot()==tg.dg.dot(), "pajek bidi inpout"

ng=BiDirectionalGraph()
ng.input("lgl","dg.lgl")
assert ng.dot()==tg.dg.dot(), "lgl bidi input"

# by generating a fully connected graph, we ensure that node
# relabelling in the LGL routines is uniquely determined.
tg.randGraph(4, 12)
tg.dg(tg.g())
ng=BiDirectionalGraph()
for fmt in ["pajek", "lgl", "dot"]:
    tg.dg.output(fmt,"g.net")
    ng.clear()
    ng.input(fmt, "g.net")
    assert tg.dg.dot()==ng.dot(), fmt


