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

LD_LIBRARY_PATH=$HOME/usr/lib:/usr/local/lib:/usr/lib:/lib
export LD_LIBRARY_PATH
$here/test/callTclArgsMethod >test.out
if test $? -ne 0; then fail; fi

cat >test.dat <<EOF
1 hello
EOF

diff test.dat test.out
if test $? -ne 0; then fail; fi

pass
