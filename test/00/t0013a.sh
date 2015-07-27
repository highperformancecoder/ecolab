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
foobar.x(3) 2
if {[foobar.x(3)]!=2} {exit 1}
foobar.x(3) 5
if {[foobar.x(3)]!=5} {exit 1}
if {[foobar.get_x 3]!=5} {exit 1}

foobar.y(3,2) 3
if {[foobar.y(3,2)]!=3} {exit 1}
foobar.y(3,2) 10
if {[foobar.y(3,2)]!=10} {exit 1}
if {[foobar.get_y 3 2]!=10} {exit 1}

set foo_called 0
foobar.z(2).foo
if {\$foo_called} {exit 0}
exit 1
EOF

$here/test/tcl-arrays input.tcl
if test $? -ne 0; then fail; fi

pass
