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
    killall mpd
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    killall mpd
    exit 0
}

trap "fail" 1 2 3 15
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# if MPICH2, then we need to run an mpd
if [ -x `which mpd` ]; then
    mpd&
fi

# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
use_namespace ecolab
parallel source model.tcl
source assert.tcl

set_grid 2 2
get 0 0
get 0 1
get 1 0
get 1 1
set initd [constant \$nsp 100]
ecolab(0,0).density \$initd
ecolab(0,1).density \$initd
ecolab(1,0).density \$initd
ecolab(1,1).density \$initd
ecolab.distribute_cells
generate
ecolab.gather
# check densities have changed
assert "!\[string equal {[ecolab(0,0).density]} {\$initd}\]"
assert "!\[string equal {[ecolab(0,1).density]} {\$initd}\]"
assert "!\[string equal {[ecolab(1,0).density]} {\$initd}\]"
assert "!\[string equal {[ecolab(1,1).density]} {\$initd}\]"

# count individual before migrate
for {set j 0} {\$j<[species.size]} {incr j} {set n0(\$j) 0}
foreach i {(0,0) (0,1) (1,0) (1,1)} {
  set d [ecolab\$i.density]
  assert "\[llength $d==[species.size]\]"
  for {set j 0} {\$j<[species.size]} {incr j} {incr n0(\$j) [lindex \$d \$j]}
}
migrate
# count individual after migrate
for {set j 0} {\$j<[species.size]} {incr j} {set n1(\$j) 0}
foreach i {(0,0) (0,1) (1,0) (1,1)} {
  set d [ecolab\$i.density]
  assert "\[llength $d==[species.size]\]"
  for {set j 0} {\$j<[species.size]} {incr j} {incr n1(\$j) [lindex \$d \$j]}
}

# check that individuals are conserved through migration
assert {[string equal [array get n0] [array get n1]]}
EOF

cp $here/models/model.tcl $here/test/assert.tcl .

# try to find MPI implementation code was built with, and run
# mpivars.sh, since AEGIS strips off unusual LD_LIBRARY_PATHS
mpicxx=`which mpicxx`
mpibin=${mpicxx%/*}
if [ -f $mpibin/mpivars.sh ]; then 
    echo "$mpibin/mpivars.sh found"
    . $mpibin/mpivars.sh
fi

mpiexec -n 2 $here/models/ecolab input.tcl
if test $? -ne 0; then fail; fi

pass
