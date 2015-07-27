#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


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
BL=`aegis -cd -bl`
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# fake a fail on -bl
if [ $here = $BL ]; then fail; fi
 
for i in $here/test/automorph-examples/*.net; do
  $here/models/netcomplexity $here/models/netcomplexity_scripts/testBDMgraph.tcl $i
if test $? -ne 0; then fail; fi
done

cd $here/models/netcomplexity_scripts
$here/models/netcomplexity celegansneural.tcl
if test $? -ne 0; then fail; fi

pass
