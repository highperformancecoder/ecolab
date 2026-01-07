#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

here=`pwd`
. $here/test/common-test.sh

$here/test/testCheckpointableFile
if test $? -ne 0; then fail; fi

diff whole.dat ckpt.dat
if test $? -ne 0; then fail; fi

cat >stream_good.dat <<EOF
hello 123
EOF

diff stream.dat stream_good.dat
if test $? -ne 0; then fail; fi

pass
