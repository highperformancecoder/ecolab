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

# check that all 3 random number libraries actually compile...

# only run  test on parallel
if [ `hostname` != "parallel" ]; then pass; fi

# insert ecolab script code here
if [ -n "$AEGIS_ARCH" ]; then
  BL=`aegis -cd -bl`
  BL1=$BL/../../baseline
  RANDOMCC=`aefind -resolve $here/src -name random.cc -print`
else #standalone test
  BL=.
  BL1=.
  RANDOMCC=$here/src/random.cc
fi

cp $RANDOMCC .
touch random_basic.cd random_unuran.cd random_gsl.cd
g++ -DTR1 -w -c -DHAVE_LONGLONG -I. -I$here/include -I$BL/include -I$BL1/include -I$HOME/usr/include random.cc
if test $? -ne 0; then echo -n "Basic: "; fail; fi
if [ "$TRAVIS" != 1 ]; then
    g++ -DTR1 -w -c -DHAVE_LONGLONG -I. -I$here/include -I$BL/include -I$BL1/include -I$HOME/usr/include -DUNURAN random.cc
    if test $? -ne 0; then echo -n "UNURAN: "; fail; fi
fi
g++ -DTR1 -w -c -DHAVE_LONGLONG -I. -I$here/include -I$BL/include -I$BL1/include -I$HOME/usr/include -DGNUSL random.cc
if test $? -ne 0; then echo -n "GNUSL: "; fail; fi

pass
