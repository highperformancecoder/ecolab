#!testCacheDBM
#  @copyright Russell Standish 2000-2013
#  @author Russell Standish
#  This file is part of Classdesc
#
#  Open source licensed under the MIT license. See LICENSE for details.

source "assert.tcl"
cDB.Init foo w
cDB.elem 10 3
cDB.elem 12 3

assert {[cDB.cacheSize]==2}
cDB.close

cDB.Init foo r
assert {[cDB.elem 10]==3 && [cDB.elem 12]==3}

#test actual caching mechanism
cDB.max_elem 3
cDB.elem 14 3
# this should flush the cache, leaving element 14 in place
cDB.elem 16 3
assert {[cDB.cacheSize]<=3}
assert {[cDB.elem 14]==3 && [cDB.elem 16]==3}

# check serialisation
cDB.checkpoint cDB.ckpt
cDB.close
cDB.restart cDB.ckpt
assert {[cDB.elem 10]==3 && [cDB.elem 12]==3 || [cDB.elem 14]==3 || [cDB.elem 16]==3}
