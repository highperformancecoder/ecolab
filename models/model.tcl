set nsp 30
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

density [constant $nsp 100]

repro_min -.1
repro_max .1
repro_rate [unirand $nsp [ecolab.repro_max] [ecolab.repro_min]]

interaction.diag [unirand $nsp -.001 -1e-3]

odiag_min -1e-5
odiag_max 1e-5
random_interaction 3 0
interaction.val [unirand [ecolab.interaction.val.size] \
   [ecolab.odiag_min] [ecolab.odiag_max]]

# mutation parameters
mut_max 1e-4
sp_sep .1
mutation [constant $nsp [ecolab.mut_max]] 

migration [constant $nsp 1e-5] 




