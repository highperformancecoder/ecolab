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
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

cat >input.tcl <<EOF
set density  {100 100} 
set repro_rate {.1 -.1}
set diag {-.0001 0}
set val {-0.001 0.001}
set row {0 1}
set col {1 0}

ecolab.density          \$density 
ecolab.repro_rate       \$repro_rate
ecolab.interaction.diag \$diag 
ecolab.interaction.val  \$val 
ecolab.interaction.row  \$row 
ecolab.interaction.col  \$col 

ecolab.checkpoint "test.ckpt"
ecolab.repro_rate ""
ecolab.interaction.val ""
ecolab.restart "test.ckpt"

proc eqarr {x y} {
    if {[llength \$x] != [llength \$y]} {return 0}
    for {set i 0; set r 1} {\$i<[llength \$x]} {incr i} {
	set r [expr \$r && [lindex \$x \$i]==[lindex \$y \$i]]
    }
    return \$r
}
        
if { [eqarr [ecolab.density]          \$density] && 
     [eqarr [ecolab.repro_rate]       \$repro_rate] &&
     [eqarr [ecolab.interaction.diag] \$diag] &&
     [eqarr [ecolab.interaction.val]  \$val] && 
     [eqarr [ecolab.interaction.row]  \$row] && 
     [eqarr [ecolab.interaction.col]  \$col] } {
	 exit 0
     } else {
	 exit 1
     }
EOF

$here/models/ecolab input.tcl
if test $? -ne 0; then fail; fi
pass
