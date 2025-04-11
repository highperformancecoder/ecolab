#! /bin/sh

here=`pwd`
. $here/test/common-test.sh

cat >input.py <<EOF
import sys
sys.path.insert(0,'$here/models')
from starComplexity import starC
nodes=4
maxStars=6

starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
starC.canonicaliseStarMap();

assert(starC.starMap()==[{'first': 0, 'second': 3}, {'first': 32, 'second': 2}, {'first': 33, 'second': 4}, {'first': 48, 'second': 3}, {'first': 50, 'second': 5}, {'first': 51, 'second': 4}, {'first': 52, 'second': 5}, {'first': 56, 'second': 1}, {'first': 60, 'second': 3}, {'first': 62, 'second': 2}, {'first': 63, 'second': 3}])

nodes=5
maxStars=7
starC.elemStars([])
starC.starMap([])
starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
starC.canonicaliseStarMap();

assert(starC.starMap()==[{'first': 0, 'second': 3}, {'first': 224, 'second': 5}, {'first': 424, 'second': 6}, {'first': 496, 'second': 6}, {'first': 504, 'second': 5}, {'first': 512, 'second': 2}, {'first': 516, 'second': 4}, {'first': 736, 'second': 6}, {'first': 768, 'second': 3}, {'first': 784, 'second': 5}, {'first': 788, 'second': 4}, {'first': 800, 'second': 5}, {'first': 896, 'second': 4}, {'first': 928, 'second': 6}, {'first': 944, 'second': 6}, {'first': 960, 'second': 1}, {'first': 992, 'second': 3}, {'first': 993, 'second': 5}, {'first': 1008, 'second': 4}, {'first': 1010, 'second': 6}, {'first': 1011, 'second': 5}, {'first': 1012, 'second': 6}, {'first': 1016, 'second': 2}, {'first': 1020, 'second': 4}, {'first': 1022, 'second': 3}, {'first': 1023, 'second': 4}])

EOF

python3 input.py
if [ $? -ne 0 ]; then fail; fi
pass
