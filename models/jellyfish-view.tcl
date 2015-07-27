#!jellyfish
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace lake

#find an unused display
#set disp_id 1
#while {[file exists /tmp/.X$disp_id-lock]} {incr disp_id}
#set Xvfb_pid [exec Xvfb :$disp_id -fp $env(HOME)/usr/lib/fonts/misc &]
#set env(DISPLAY) localhost:$disp_id

Tkinit
source $argv(1)

set lake [image create photo -file $lakef]
if [file exists "[file rootname $lakef].bin"] {
  parallel lake.restart [file rootname $lakef].bin
} else {
  lake.set_lake $lake
}



GUI

toplevel .lake
canvas .lake.canvas -height [image height $lake] -width [image width $lake]
pack .lake.canvas
.lake.canvas create image 0 0 -anchor nw -image $lake
.lake.canvas create bitmap 0 0 -anchor nw -foreground #0000C4 -tag shadow

# allow selection of individual jellyfish with a mouse
bind .lake.canvas <Button-1> {
    set obj "[lake.select %x %y]"
    if {[string length $obj]==0} return
    obj_browser [lake.select %x %y]*
    lake.draw .lake.canvas
}

bind .lake.canvas <Button-3> {
    if {[info commands ::depths::clear]=="::depths::clear"} {::depths::clear} 
    histogram depths [depthlist %x %y]
# why is this needed
    ::depths::reread 
}

.user1 configure -text "load checkpoint" -command {lake.restart [tk_getOpenFile -filetypes {{checkpoint {*.ckpt}}}]; lake.draw .lake.canvas}

focus .

if {$argc>2} {lake.restart $argv(2).ckpt}
lake.draw .lake.canvas
update
#exec xwd -id [winfo id .lake] -out $argv(2).xwd
#exec kill $Xvfb_pid &
#exit_ecolab
