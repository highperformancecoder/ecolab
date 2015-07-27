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
    killall /usr/bin/X11/Xvfb
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    cd $here
    chmod -R u+w $tmp
    rm -rf $tmp
    killall /usr/bin/X11/Xvfb
    exit 0
}

trap "fail" 1 2 3 15

if [ -n "$AEGIS_ARCH" ]; then
  XECOLAB=`aefind -resolve $here/include -name Xecolab.tcl -print`
else
  XECOLAB=$here/include/Xecolab.tcl
fi

# insert ecolab script code here
# use \$ in place of $ to refer to variable contents
# exit 0 to indicate pass, and exit 1 to indicate failure
cat >input.tcl <<EOF
set ecolab_library [file dirname $XECOLAB]
GUI

urand gen
gen.set_gen mt19937(19863)
histogram ran [gen.rand]
histogram ran [gen.rand]
exit_ecolab
EOF

# for some reason this test fails at integration
# crude test of being in integration mode
if [ "${here%delta*}" != $here ]; then pass; fi
if [ ! -x /usr/bin/X11/Xvfb ]; then pass; fi
DISPLAY=:30
export DISPLAY
killall /usr/bin/X11/Xvfb
/usr/bin/X11/Xvfb $DISPLAY &
#startx -- /usr/bin/Xvfb :2&
sleep 1
$here/models/ecolab input.tcl
if test $? -ne 0; then fail; fi

pass





