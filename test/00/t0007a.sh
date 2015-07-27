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
# don't run this test is Aegis not present
if [ -z "$AEGIS_ARCH" ]; then pass; fi

BL=`aegis -cd -bl`
BL4=`aegis -cd -bl -p ecolab.5`

# Check that all included files in Xecolab are in the distribution
XECOLAB=$here/include/Xecolab.tcl
if [ ! -f $XECOLAB ]; then XECOLAB=$BL/include/Xecolab.tcl; fi
if [ ! -f $XECOLAB ]; then XECOLAB=$BL4/include/Xecolab.tcl; fi
grep "source \$ecolab" $XECOLAB|cut -f3 -d/|sort|uniq >file1
aegis -list pf|grep include/Xecolab/|cut -f3 -d/ >file2
aegis -list cf|grep include/Xecolab/|cut -f3 -d/ >>file2
sort file1|uniq>file3
sort file2|uniq>file4
diff file3 file4
if test $? -ne 0; then fail; fi

pass
