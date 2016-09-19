#! /bin/sh
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.



fail()
{
    echo "FAILED" 1>&2
    exit 1
}

pass()
{
    echo "PASSED" 1>&2
    exit 0
}

trap "fail" 1 2 3 15

test/test_omp_rw_lock
if test $? -ne 0; then fail; fi

pass
