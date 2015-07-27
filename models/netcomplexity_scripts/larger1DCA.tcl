#!../netcomplexity
use_namespace netc
HistoStats Rewire

large1DCA.cells.resize 10
large1DCA.nStates 4
large1DCA.nbhdSz 5
large1DCA.uni.seed [expr [clock microseconds] % 8192]

for {set ii 0} {$ii<1000} {incr ii} {
    set lambda [expr ([large1DCA.uni.rand]-0.5)*0.2+0.55]
    large1DCA.initRandomRule $lambda

    large1DCA.fillNet 10000 5000

    if {[large1DCA.net.links]<2} continue
    set fmt "%3.1f"
    #puts -nonewline stdout "[exec date] "
    puts -nonewline stdout "[format %4.2f [large1DCA.lambda]] [large1DCA.net.nodes] [large1DCA.net.links] "
    flush stdout

    if {[large1DCA.net.links]>[large1DCA.net.nodes]*([large1DCA.net.nodes]-1)} {
        large1DCA.net.output pajek bad.net
        exit
    }

    set complexity [large1DCA.complexity]
    puts -nonewline stdout "[format $fmt $complexity] "
    flush stdout

    # randomly shuffle links
    Rewire.clear
    for {set i 1} {$i<100} {incr i} {
        large1DCA.random_rewire
        Rewire.add_data [expr log([large1DCA.complexity])]
    }
 
    puts -nonewline stdout "[format $fmt [expr exp([Rewire.av])]] [format $fmt [expr $complexity-exp([Rewire.av])]] "
    flush stdout
    if {[Rewire.stddev] > 0} {
        puts stdout "[format $fmt [expr abs(log($complexity)-[Rewire.av])/[Rewire.stddev]]]"
    } elseif {abs([Rewire.av]-log($complexity))<1} {
        puts 0
    } else {puts stdout "infinity"}
}
