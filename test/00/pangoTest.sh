#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
. $here/test/common-test.sh

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
