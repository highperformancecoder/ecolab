#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


here=`pwd`
. $here/test/common-test.sh

# Test condense functionality
cat >input.py <<EOF
from ecolab_model import panmictic_ecolab as ecolab
ecolab.density([1, 0, 1, 0, 1])
ecolab.makeConsistent()
ecolab.repro_rate([1, 1, 1, 1, 1])
ecolab.interaction.diag([1, 1, 1, 1, 1])
ecolab.condense()
if ecolab.density()!=[1, 1, 1]: exit(1)
EOF

$here/bin/ecolab input.py
if test $? -ne 0; then fail; fi

pass
