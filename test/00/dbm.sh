#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
. $here/test/common-test.sh

$here/test/testNDBM
if test $? -ne 0; then fail; fi

$here/test/testBDB
if test $? -ne 0; then fail; fi

pass
