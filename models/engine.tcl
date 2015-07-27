#!ecolab
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.


ecolab.density {100 100} 
ecolab.repro_rate {.1 -.1}
ecolab.interaction.diag {-.0001 0}
ecolab.interaction.val {-0.00105 0.00105}
ecolab.interaction.row {0 1}
ecolab.interaction.col {1 0}

proc send_density {channel client port} {
    puts stdout "$channel [ecolab.tstep] [ecolab.density]"
    puts $channel "[ecolab.tstep] [ecolab.density]"
    close $channel
}

ecolab.data_server 7000
socket -server send_density 7001

while 1 { 
    ecolab.generate
    if {[ecolab.tstep] % 1000 == 0} {puts stdout [ecolab.density]}
}

