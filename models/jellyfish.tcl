#!jellyfish
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

parallel use_namespace lake
Tkinit

if {$argc>1} {
    source $argv(1)
} else {
    set lakef "lake.gif"
    set scale 1
    mangrove_height 3
}


set lake [image create photo -file $lakef]
if [file exists "[file rootname $lakef].bin"] {
  parallel restart [file rootname $lakef].bin
} else {
  set_lake $lake
}

set time 7
set day 1
set month 0
set latitude 7

parallel mangrove_height [lake.mangrove_height]
parallel uni.seed {[expr 100*([myid]+1)]}
parallel gaussrand gran
parallel gran.uni.seed {[expr 100*([myid]+1)]}
parallel speed_dist.set_gen gran
parallel angle_dist.set_gen gran
parallel photo_taxis .1
# turning probability
parallel change_prob .8

proc set_tstep {newval} {
    global tstep scale
    #timestep in seconds
    set tstep $newval
    
    # jellyfish mean speed in metres per timestep
    parallel speed_dist.scale [expr 0.06*$scale*$tstep]
    parallel max_speed [expr 3*[speed_dist.scale]]
    parallel vz_dist.scale [expr 0.005*$scale*$tstep]

# set the number of cells in each direction for the jellyfish neighbourhood map
    parallel nmapx [expr int([width] / ([speed_dist.scale]*6.0))]
    parallel nmapy [expr int([height] /([speed_dist.scale]*6.0))]
    parallel nmapz [expr int([depth] /([vz_dist.scale]*5))]
#    parallel nmapx 1
#    parallel nmapy 3
    parallel nmapz 1
    puts stdout "[nmapx] [nmapy] [nmapz]"
    init_map       

}

# set initial timestep
set_tstep 10

# jellyfish mean turn angles (degrees)
parallel angle_dist.scale 24

# 3D model parameters
parallel depth [expr 10*$scale]
parallel ideal_depth [expr -1*$scale]
parallel vert_change_prob 0.2
parallel vz_dist.set_gen gran

# discrete distribution of bell sizes (in cms)
# distribution is only used on master processor
unuran rdist
rdist.set_gen {distr=discr; pv=( 20.62, 20.91, 17.10, 8.98, 6.00, 4.48, 3.28, 2.98, 2.95, 3.08, 3.10, 2.84, 1.93, 1.00, 0.52, 0.18, 0.04, 0.01)}
#urand rdist
radii_dist.set_gen rdist
# scale distribution to model scale
radii_dist.scale 0.01*$scale

parallel puts "init finished [myid] [nprocs]"

# <no. jellies> 
add_jellyfish 2000

parallel colourprocs 0
set palette {black red green magenta cyan yellow pink orange}

set display_style flow
proc draw {} {
    global display_style
    if {$display_style=="density"} {
        lake.draw_density .lake.canvas.density
    } else {
        lake.draw .lake.canvas
    }
}

set upd_ctr 0

proc step {} {
    uplevel #0 { 
    .statusbar configure -text "[time $time] [date [dayofyear $day $month]] cell max:[cell_max]"
    puts "[time $time] [cputime]"
    if {abs($time-7.5)<=1.5 || abs($time-17)<=1} {
        # update shadow lines every 10 seconds
        set shad_update_freq [expr 10/$tstep]
    } else {
        # update shadow lines every 6 minutes
        set shad_update_freq [expr 360 / $tstep]
    }
    for {set i 0} {$i<$shad_update_freq} {incr i} {
        lake.update
        incr upd_ctr
        if {$upd_ctr%10==0} {
            puts stdout $upd_ctr
            lake.update_map
            draw
        }
    }
    set time [expr $time+$shad_update_freq*$tstep/3600.0]
    if {$time>=24} {
        set time [expr fmod($time,24)]
        incr day
    }
    eval compute_shadow [sun_angle]
}
}

proc simulate {} { 
    global running
    set running 1
    for {} {$running} { } {step}
}

# date = day in year
# time = hours (24 hour clock)
proc sun_angle {} {
    global latitude time day month
    set date [dayofyear $day $month]
    set pi2 6.2831853
    set theta [expr ($time/24.0+.5)*$pi2]
    set phi [expr sin(($date-110)/365.0*$pi2)*23.0/360.0*$pi2]
    set alpha [expr $latitude/360.0*$pi2]
    return [list [expr -cos($phi)*sin($theta)] \
	 [expr -cos($phi)*cos($theta)*sin($alpha)+sin($phi)*cos($alpha)]\
	 [expr cos($phi)*cos($theta)*cos($alpha)+sin($phi)*sin($alpha)]\
    ]
}


proc time {t} {return [expr int($t)]:[format "%02d" [expr int(fmod($t*60,60))]]}

GUI

eval compute_shadow [sun_angle]


toplevel .lake
canvas .lake.canvas -height [image height $lake] -width [image width $lake]
pack .lake.canvas
.lake.canvas create image 0 0 -anchor nw -image $lake
.lake.canvas create bitmap 0 0 -anchor nw -foreground #0000C4 -tag shadow
image create photo .lake.canvas.density -height [image height $lake] \
    -width [image width $lake]
.lake.canvas create image 0 0 -anchor nw -image .lake.canvas.density
draw

toplevel .setdate
scale .setdate.settime -from 0 -to 24 -variable time -resolution 1 \
	-orient horizontal -label time
scale .setdate.setday -from 1 -to 31 -variable day -resolution 1 \
	-orient horizontal -label day
frame .setdate.buttons 
menubutton .setdate.buttons.monthmenu -direction below -text [month $month] -relief raised
button .setdate.buttons.update -command {eval compute_shadow [sun_angle]
    draw} -text update
pack  .setdate.buttons.monthmenu .setdate.buttons.update  -side left 
pack .setdate.settime .setdate.setday  .setdate.buttons 

menu .setdate.buttons.monthmenu.setmonth
.setdate.buttons.monthmenu configure -menu .setdate.buttons.monthmenu.setmonth 
.setdate.buttons.monthmenu.setmonth add radiobutton -value 0 -label Jan \
    -variable month -command {.setdate.buttons.monthmenu configure -text Jan}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 1 -label Feb\
    -variable month -command {.setdate.buttons.monthmenu configure -text Feb}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 2 -label Mar\
    -variable month -command {.setdate.buttons.monthmenu configure -text Mar}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 3 -label Apr\
    -variable month -command {.setdate.buttons.monthmenu configure -text Apr}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 4 -label May\
    -variable month -command {.setdate.buttons.monthmenu configure -text May}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 5 -label Jun\
    -variable month -command {.setdate.buttons.monthmenu configure -text Jun}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 6 -label Jul\
    -variable month -command {.setdate.buttons.monthmenu configure -text Jul}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 7 -label Aug\
    -variable month -command {.setdate.buttons.monthmenu configure -text Aug}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 8 -label Sep\
    -variable month -command {.setdate.buttons.monthmenu configure -text Sep}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 9 -label Oct\
    -variable month -command {.setdate.buttons.monthmenu configure -text Oct}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 10 -label Nov\
    -variable month -command {.setdate.buttons.monthmenu configure -text Nov}
.setdate.buttons.monthmenu.setmonth add radiobutton -value 11 -label Dec\
    -variable month -command {.setdate.buttons.monthmenu configure -text Dec}

proc setcontrast x {
    lake.contrast $x
    draw
}

scale .buttonbar.contrast -from 1 -to 100 -command lake.contrast -orient horizontal -label contrast
pack .buttonbar.contrast -side right

# allow selection of individual jellyfish with a mouse
bind .lake.canvas <Button-1> {
    set obj "[lake.select %x %y]"
    if {[string length $obj]==0} return
    obj_browser $obj.*
    draw 
}

bind .lake.canvas <Button-3> {
    if {[info commands ::depths::clear]=="::depths::clear"} {::depths::clear} 
    histogram depths [depthlist %x %y]
# why is this needed
    ::depths::reread 
}

.user1 configure -text "density" -command {
    if {$display_style=="flow"} {
        .lake.canvas delete jfish shadow
        set display_style density
        .user1 configure -text flow
    } else {
        set display_style flow
        .user1 configure -text density
    }
    draw
}

.user2 configure -text "step" -command {lake.update; draw}

focus .
