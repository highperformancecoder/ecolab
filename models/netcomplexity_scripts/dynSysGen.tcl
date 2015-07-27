#!../netcomplexity
use_namespace netc
HistoStats Rewire

# set the coarse-grain grid size
lorenzGen.n {20 20 20}
henonHeilesGen.n {10 10 10 10}

foreach sys {lorenzGen henonHeilesGen} {
    $sys.fillNet 20000
    puts -nonewline stdout "$sys [$sys.net.nodes] [$sys.net.links] "
    set complexity [$sys.complexity]
    set fmt "%3.1f"
    puts -nonewline stdout "[format $fmt $complexity] "
    flush stdout
    
    # randomly shuffle links
    Rewire.clear
    for {set i 1} {$i<1000} {incr i} {
        $sys.random_rewire
        Rewire.add_data [expr log([$sys.complexity])]
    }
    
    puts -nonewline stdout "[format $fmt [expr exp([Rewire.av])]] [format $fmt [expr $complexity-exp([Rewire.av])]] "
    flush stdout
    if {[Rewire.stddev] > 0} {
        puts stdout "[format $fmt [expr abs(log($complexity)-[Rewire.av])/[Rewire.stddev]]]"
    } else {puts stdout "infinity"}
}

