# plot widget definitions
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


proc plot {name args} {
    if {![winfo exists .$name]} {
	namespace eval $name {
	    # Used to format the x tick labels
	    proc fmttick {name val} {
		return [format "%6.1f" $val]
	    }

	    proc init {name title} {
		global tcl_platform
		variable xdata
		variable xname
		toplevel .$name
		wm title .$name $title
                
                if  {[lsearch -exact [image names] .$name.image]!=-1} {
                    image delete .$name.image
                }
                image create photo .$name.image  -width 500 -height 500
                Plot .$name.plot
                .$name.plot.image .$name.image  0
                .$name.plot.plotType 0
                label .$name.graph -image .$name.image 

		frame .$name.buttonbar
		button .${name}.buttonbar.print -text Print -command [namespace code \
		"print \[tk_getSaveFile -defaultextension \".ps\" -parent .$name\]"
		] -state disabled
		button .${name}.buttonbar.clear -text Clear -command .$name.plot.clear

		pack append .$name.buttonbar .$name.buttonbar.print left \
		    .$name.buttonbar.clear left
		pack append .$name .$name.buttonbar top
		pack append .$name .$name.graph top

	    }

	    proc print {{filename plot.ps } {options ""}} {
		eval .[namespace tail [namespace current]].graph \
			postscript output $filename $options
	    }
	    
	    proc plot {name arglist} {
		variable xdata
		variable lastx 
		variable xname

		# grab out title argument and remove title from args list
		if {[lindex $arglist 0] == "-title"} {
		    set title [lindex $arglist 1]
		    set arglist [lreplace $arglist 0 1]
		} else {
		    set title $name
		}

		if [string_is double [lindex $arglist 0]] {
		    # numerical data passed for x - just call it "x"
		    set xname x
		    set xvalue [lindex $arglist 0]
		} else {
		    # x component is a named variable
		    set xname [lindex $arglist 0]
		    upvar #0 $xname xvalue
		}

		if {![winfo exists .$name]} {
		    set lastx [expr $xvalue-1] 
		    init $name $title
		}

		# don't do unnecessary work
		if {$xvalue==$lastx} return
		set lastx $xvalue
	    
		for {set i 1} {$i<[llength $arglist]} {incr i} {
		    if [string_is double [lindex $arglist $i]] {
			# grab numerical values passed, and name them y1...yn
			set argname y[set i]
			lappend value [lindex $arglist $i]
		    } else {
			# declare all named arglist variable names as global
			set argname [lindex $arglist $i]
			upvar #0 $argname v
                        lappend value $v
		    }
                }
                .$name.plot.plot $xvalue $value
		
		
	    }

	    
	}
    }
    [set name]::plot $name $args
    return $name
}
