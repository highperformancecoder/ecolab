# display widget definitions
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


proc display {args} {

    if {[lindex $args 0]=="-title"} {
	set title [lindex $args 1]
	set args [lreplace $args 0 1]
    } else {
	set title [lindex $args 1]
    }
	
    set name [string_map {. _ , _ ( _ ) _} "display_[lindex $args 1]"]
    if {![winfo exists .$name]} {
	toplevel .$name 
	wm title .$name $title

        if  {[lsearch -exact [image names] .$name.image]!=-1} {
            image delete .$name.image
        }
        image create photo .$name.image  -width 500 -height 500
	Plot .${name}.plot 
        .$name.plot.image .$name.image  0
        label .$name.graph -image .$name.image 
	global tcl_platform
	frame .$name.buttonbar
	button .${name}.buttonbar.print -text Print -command "
                ${name}::print \[tk_getSaveFile -defaultextension \".ps\" \
		-parent .$name\]" -state disabled
	button .${name}.buttonbar.clear -text Clear -command .$name.plot.clear
	pack append .$name.buttonbar .$name.buttonbar.print left \
	    .$name.buttonbar.clear left
	pack append .$name .$name.buttonbar top
	pack append .${name} .${name}.graph top
	
	namespace eval $name {
	    variable time
	    
	    proc print {{filename display.ps } {options ""}} {
		eval .[namespace tail [namespace current]].graph \
			postscript output $filename $options
	    }

	}
	
    }
    eval eco_display $args
    return $name
}
