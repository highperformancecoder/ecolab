#!jellyfish
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

Tkinit
source $argv(1)
lake.set_lake [image create photo -file $lakef]
lake.checkpoint $argv(1).bin
exit
