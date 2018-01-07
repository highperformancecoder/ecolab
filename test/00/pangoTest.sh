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

rm $here/test/pangoTest.o
make -s -C $here/test PANGO=1 pangoTest
if test $? -ne 0; then fail; fi
$here/test/pangoTest
rm $here/test/pangoTest.o
if test $? -ne 0; then fail; fi
make -s -C $here/test PANGO= pangoTest
if test $? -ne 0; then fail; fi
$here/test/pangoTest
if test $? -ne 0; then fail; fi


pass
