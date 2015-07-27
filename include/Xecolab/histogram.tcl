
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

# histogram plot

proc histogram {name args} {
    namespace eval $name {
	if {![llength [info commands init]]} {

	    proc setnbins {x} {
		set name [namespace tail [namespace current]]
		if {$x==[hist.nbins]} return
		hist.nbins [expr int(exp($x/10.0))]
		.$name.nbincontrol configure -label "No. Bins:[hist.nbins]"
		after 100 [namespace code hist.reread]
	    }

	    proc xlogscale {} {
		set name [namespace tail [namespace current]]
		if {[hist.logbins]} {
		    .$name.xlogscale configure -relief raised
		    hist.logbins 0
                    hist.logx 0
		} else {
		    .$name.xlogscale configure -relief sunken
		    hist.logbins 1
                    hist.logx 1
		}
		hist.reread
	    }
	    
	    proc ylogscale {} {
		set name [namespace tail [namespace current]]
		if [hist.logy] {
		    .$name.ylogscale configure -relief raised
                    hist.logy 0
		} else {
		    .$name.ylogscale configure -relief sunken
                    hist.logy 1
		}
                hist.reread
	    }

	    # Used to format the x tick labels
	    proc fmttick {name val} {
		return [format "%6.2g" $val]
	    }

	    proc outputdata {} {
		set name [namespace tail [namespace current]]
		set fname [tk_getSaveFile -defaultextension \".dat\" \
			       -parent .$name]
		if {$fname==""} return
		hist.outputdat $fname
	    }

	    proc print {{filename histogram.ps } {options ""}} {
		eval .[namespace tail [namespace current]].graph \
		    postscript output $filename $options
	    }

            proc doClear {} {
                hist.clear
                #not sure why we need to do this!
                if [hist.logy] ylogscale
            }

	    proc histogram {name argl} {

		# grab out title argument and remove title from args list
		if {[lindex $argl 0] == "-title"} {
		    set title [lindex $argl 1]
		    set value [lrange $argl 2 end]
		} else {
		    set title $name
		    set value $argl
		}

		if {![winfo exists .$name]} {
		    toplevel .$name

		    # create the C++ histogram object
                    if  {[lsearch -exact [image names] .$name.image]!=-1} {
                        image delete .$name.image
                    }
                    image create photo .$name.image  -width 500 -height 500
		    HistoGram [namespace current]::hist
		    # use_namespace command dumps into global namespace, 
		    # this dumps into current namespace
		    #foreach comm [info commands hist.*] {
		    #    regsub {^[^.]*\.} $comm "" procname
		    #    proc $procname {args} "eval $comm \$args"
		    #}
                    hist.image .$name.image 0
		    
		    hist.nbins 99
		    hist.logbins 0
		    hist.max -1E38
		    hist.min 1E38

		    # log scale controls
		    frame .$name.buttonbar 
		    pack .$name.buttonbar -fill x -side top -in .$name 
		    button .$name.xlogscale -text "x logscale" \
			-command [namespace code "xlogscale"]
		    button .$name.ylogscale -text "y logscale" \
			-command [namespace code "ylogscale"]
		    
		    button .${name}.print -text Print -command [namespace code \
		       "print \[tk_getSaveFile -defaultextension \".ps\" \
			    -parent .$name\]"
		       ] -state disabled
		    button .${name}.clear -text Clear -command [namespace code doClear]
		    pack append .$name.buttonbar .$name.xlogscale left \
			.$name.ylogscale left .${name}.print left .${name}.clear left 

		    # Output histogram data
		    button .$name.outputdata -text "Output Histogram" \
			-command [namespace code "outputdata"]
		    pack append .$name.buttonbar .$name.outputdata left


		    label .$name.graph -image .$name.image 
		    pack append .$name .$name.graph left

		    # Scale label for x-axis
		    label .$name.xscale 
		    pack append .$name .$name.xscale bottom
		    
		    # scroll widget to control number of bins
		    scale .$name.nbincontrol -from 0 -to [expr [hist.nbins]+2] \
			-showvalue false\
			-command "[namespace code setnbins]"
		    .$name.nbincontrol set 46
		    pack append .$name .$name.nbincontrol {right filly}
		}

                hist.add_data $value

	    }
	}
    }
    [set name]::histogram $name $args
    return $name
}
