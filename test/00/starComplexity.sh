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

assert starC.starMap.keys()==[0, 32, 33, 48, 50, 51, 52, 56, 60, 62, 63]
starMapRef={0: 3, 32: 2, 33: 4, 48: 3, 50: 5, 51: 4, 52: 5, 56: 1, 60: 3, 62: 2, 63: 3}
for i in starC.starMap.keys():
    assert starC.starMap[i].star()==starMapRef[i]

nodes=5
maxStars=6
starC.elemStars([])
starC.starMap([])
starC.generateElementaryStars(nodes)
starC.fillStarMap(maxStars)
starC.canonicaliseStarMap();

assert starC.starMap.keys()==[0, 224, 424, 496, 504, 512, 516, 736, 768, 784, 788, 800, 896, 928, 944, 960, 992, 993, 1008, 1010, 1011, 1012, 1016, 1020, 1022, 1023]

starMapRef={0: 3, 224: 5, 424: 6, 496: 6, 504: 5, 512: 2, 516: 4, 736: 6, 768: 3, 784: 5, 788: 4, 800: 5, 896: 4, 928: 6, 944: 6, 960: 1, 992: 3, 993: 5, 1008: 4, 1010: 6, 1011: 5, 1012: 6, 1016: 2, 1020: 4, 1022: 3, 1023: 4}
for i in starC.starMap.keys():
    assert starC.starMap[i].star()==starMapRef[i]
EOF

python3 input.py
if [ $? -ne 0 ]; then fail; fi
pass
