#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
if test $? -ne 0; then exit 2; fi
tmp=/tmp/$$
mkdir $tmp
if test $? -ne 0; then exit 2; fi
cd $tmp
if test $? -ne 0; then exit 2; fi

fail()
{
    echo "FAILED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    exit 0
}

trap "fail" 1 2 3 15
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
source $here/test/assert.tcl

HistoStats h


unuran rand
rand.set_gen {distr=pareto(.5,1.7);}
rand.uni.seed 10

for {set i 0} {\$i<10000} {incr i} {
    h.add_data [rand.rand]
}

proc min {x y} {
    if {\$x<\$y} {return \$x} else {return \$y}
}

set plparms [h.fitPowerLaw]
set slope [lindex \$plparms 0]
set xmin [expr [min [lindex \$plparms 1] [h.av]]]

assert {abs(\$slope-2.7)<0.1 && abs(\$xmin-0.5)<0.1}

set lnparms [h.fitLogNormal]
set mu [lindex \$lnparms 0]
set sigma [lindex \$lnparms 1]

set expparms [h.fitExponential]

assert {[h.loglikelihood powerlaw(\$slope,\$xmin) lognormal(\$mu,\$sigma) \$xmin]>0}
    assert {[h.loglikelihood powerlaw(\$slope,\$xmin) exponential(\$expparms) \$xmin]>0}

h.clear
rand.set_gen {distr=exponential(0.5)}

for {set i 0} {\$i<10000} {incr i} {
    h.add_data [rand.rand]
}

set plparms [h.fitPowerLaw]
set slope [lindex \$plparms 0]
set xmin [expr [min [lindex \$plparms 1] [h.av]]]

set lnparms [h.fitLogNormal]
set mu [lindex \$lnparms 0]

set sigma [lindex \$lnparms 1]

set expparms [h.fitExponential]
assert {abs(\$expparms-0.5)<0.1}

assert {[h.loglikelihood exponential(\$expparms) powerlaw(\$slope,\$xmin) \$xmin]>0}
assert {[h.loglikelihood exponential(\$expparms) lognormal(\$mu,\$sigma) \$xmin]>0}

h.clear
rand.set_gen {distr=normal(1,3)}

for {set i 0} {\$i<10000} {incr i} {
    h.add_data [expr exp([rand.rand])]
}

set plparms [h.fitPowerLaw]
set slope [lindex \$plparms 0]
set xmin [expr [min [lindex \$plparms 1] [h.av]]]

set lnparms [h.fitLogNormal]
set mu [lindex \$lnparms 0]
set sigma [lindex \$lnparms 1]
assert {abs(\$mu-1)<0.1 && abs(\$sigma-3)<0.1}

set expparms [h.fitExponential]

assert {[h.loglikelihood lognormal(\$mu,\$sigma) powerlaw(\$slope,\$xmin) \$xmin]>0}
assert {[h.loglikelihood lognormal(\$mu,\$sigma) exponential(\$expparms) \$xmin]>0}

h.clear
rand.set_gen {distr=normal(1,3)}

for {set i 0} {\$i<10000} {incr i} {
    h.add_data [expr [rand.rand]]
}

set plparms [h.fitPowerLaw]
set slope [lindex \$plparms 0]
set xmin [expr [min [lindex \$plparms 1] [h.av]]]

set nparms [h.fitNormal]
set mu [lindex \$nparms 0]
set sigma [lindex \$nparms 1]
assert {abs(\$mu-1)<0.1 && abs(\$sigma-3)<0.1}

set expparms [h.fitExponential]

assert {[h.loglikelihood normal(\$mu,\$sigma) powerlaw(\$slope,\$xmin) \$xmin]>0}
assert {[h.loglikelihood normal(\$mu,\$sigma) exponential(\$expparms) \$xmin]>0}


EOF

$here/models/ecolab input.tcl
if test $? -ne 0; then fail; fi

pass
