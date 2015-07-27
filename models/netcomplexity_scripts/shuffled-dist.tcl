#!../netcomplexity
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

use_namespace netc
g.input pajek $argv(1)
puts stdout [complexity]
HistoStats Rewire
HistoStats logRewire
for {set i 1} {$i<10000} {incr i} {
    random_rewire
    Rewire.add_data [expr [complexity]]
    logRewire.add_data [expr log([complexity])]
}
#GUI
#histogram h [Rewire]
# uncomment to get logarithmic x scale
#Rewire.logbins 1
set outdat [open "shuffled-compexity.histodat" w]
foreach x [Rewire.bins] y [Rewire.histogram] {
    puts $outdat "$x $y"
} 
close $outdat

puts "avlog=[logRewire.av], stddev=[logRewire.stddev]"
set normdist "normal([Rewire.av],[Rewire.stddev])"
set lognormdist "lognormal([regsub -all { } [Rewire.fitLogNormal] {,}])"
puts -nonewline stdout "loglikelihood $normdist / $lognormdist ="
flush stdout
puts [Rewire.loglikelihood $normdist $lognormdist [Rewire.min]] 
GUI
