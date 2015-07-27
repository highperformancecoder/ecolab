#!../netcomplexity
use_namespace netc
HistoStats Rewire

small1DCA.nCells 20
small1DCA.uni.seed [expr [clock microseconds] % 8192]
for {set rule 0} {$rule <256} {incr rule} {
    small1DCA.rule $rule
    for {set iter 0} {$iter<1} {incr iter} {
        small1DCA.fillNet 10000 1000
        if {[small1DCA.net.links]==0} continue
        set fmt "%3.1f"
        puts -nonewline stdout "1DCA $rule [format %4.2f [small1DCA.lambda]] [small1DCA.net.nodes] [small1DCA.net.links] "
        set complexity [small1DCA.complexity]
        puts -nonewline stdout "[format $fmt $complexity] "
        flush stdout

        if {$complexity==0} {continue}
        
        # randomly shuffle links
        Rewire.clear
        for {set i 1} {$i<100} {incr i} {
            small1DCA.random_rewire
            Rewire.add_data [expr log([small1DCA.complexity])]
        }
    
        puts -nonewline stdout "[format $fmt [expr exp([Rewire.av])]] [format $fmt [expr $complexity-exp([Rewire.av])]] "
        flush stdout
        if {[Rewire.stddev] > 0} {
            puts stdout "[format $fmt [expr abs(log($complexity)-[Rewire.av])/[Rewire.stddev]]]"
        } else {puts stdout "infinity"}
    }
}
