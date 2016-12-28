#!stupid06
use_namespace stupidModel

set log [open stupid.log w]


set toroidal 1
set nx 100
set ny 100
set mv_dist 4
set max_food_production 0.1
setup $nx $ny $mv_dist $toroidal $max_food_production

set nbugs 100
set max_food_consumption 1
addBugs $nbugs $max_food_consumption

set maxsize 1
for {} {$maxsize<100} { } {
    grow
    moveBugs
    set bs [bugsizes]
    set maxsize [max $bs]
    puts $log "[tstep] [min $bs] [av $bs] $maxsize"
}

close $log



