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
cat >input.py <<EOF
array_urand.seed(10)
ecolab.species([1, 2])
ecolab.density([100, 100]) 
ecolab.create([0, 0])
ecolab.repro_rate([.1, -.1])
ecolab.interaction.diag([-.0001, -1e-5])
ecolab.interaction.val([-0.001, 0.001])
ecolab.interaction.row([0, 1])
ecolab.interaction.col(1, 0])
ecolab.mutation([.01, .01])
ecolab.sp_sep(.1)
ecolab.repro_min(-.1)
ecolab.repro_max(.1)
ecolab.odiag_min(-1e-3)
ecolab.odiag_max(1e-3)
ecolab.mut_max(.01)

with open("out.dat","w") as output:
     for j in range(10):
        for i in range(100):
	    ecolab.generate()
	ecolab.condense()
        print(ecolab.density(), file=output)

EOF

$here/models/ecolab input.tcl
if test $? -ne 0; then fail; fi

traceOption=0
if uname -a|grep Ubuntu; then
# for Travis
    traceOption=1
fi

case $traceOption in
    0)
cat >out1.dat <<EOF
102 85
99 87
105 93
97 89
98 99
102 79
97 101
110 82
98 87
103 91
EOF
;;
    1)
cat >out1.dat <<EOF
106 79
90 99
112 82
90 96
103 84
100 101
97 82
98 108
100 70
93 106
EOF
;;
    2)
cat >out1.dat <<EOF
105 79
96 98
100 82
102 98
100 87
96 90
106 88
99 82
103 98
99 78
EOF
;;
esac

if diff out.dat out1.dat; then 
pass
else
fail
fi;

pass
