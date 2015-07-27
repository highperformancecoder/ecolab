#! /bin/sh

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
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

cat >1Dex.dat <<EOF
0
5
3
5
3
4
EOF

cat >2Dex.dat <<EOF
1 4
5 5
3 2
5 1
4 4
EOF

cat >3Dex.dat <<EOF
1 1 4
5 1 5
3 1 2
5 1 1
4 1 4
EOF




# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
source assert.tcl
NetworkFromTimeSeries nw
# check 1D load
nw.resize 1
nw.loadData 1Dex.dat
nw.@elem 0
assert {[nw(0)]=="0 5 3 5 3 4"}

# check 1D case, resolution 1
nw.n 6
nw.constructNet
assert {[nw.net]=="digraph {
0->5;
3->4;
3->5;
5->3\[weight = 2\];
}
"}

# check 1D case, resolution 2
nw.n 3
nw.constructNet
assert {[nw.net]=="digraph {
0->2;
1->2\[weight = 2\];
2->1\[weight = 2\];
}
"}

# check 2D load
nw.clear
nw.resize 2
nw.loadData 2Dex.dat
nw.@elem 0
nw.@elem 1
assert {[nw(0)]=="1 5 3 5 4"}
assert {[nw(1)]=="4 5 2 1 4"}

nw.n {5 5}
nw.constructNet
assert {[nw.net]=="digraph {
4->18;
7->4;
15->24;
24->7;
}
"}

# check column indexing
set savedNet [nw.net]
nw.columnIdx {0 2}
nw.clear
nw.resize 2
nw.loadData 3Dex.dat
nw.constructNet
assert {[nw.net]==\$savedNet}

EOF

cp $here/test/assert.tcl .
# test disabled, because we're using Small's algorithm now.
#$here/models/ecolab input.tcl
if test $? -ne 0; then fail; fi

pass
