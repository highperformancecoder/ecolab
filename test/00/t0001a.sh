#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
. $here/test/common-test.sh

#LD_LIBRARY_PATH=$HOME/usr/lib:/usr/local/lib:/usr/lib:/lib
export LD_LIBRARY_PATH
$here/test/test_ecostrstream >test.out
if test $? -ne 0; then fail; fi

cat >test.dat <<EOF
hello to a world 1234567890 4.56123
var123
EOF

diff test.dat test.out
if test $? -ne 0; then fail; fi

pass
