#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
. $here/test/common-test.sh


# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.py <<EOF
from ecolab import unuran
rand=unuran()
rand.setGen('distr=cont; pdf="x^2"; domain=(0,1)')

for i in range(10): rand.rand()
EOF

$here/bin/ecolab input.py
if test $? -ne 0; then fail; fi

pass
