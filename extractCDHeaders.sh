#!/bin/bash
# Clunky, but this extracts all .cd files mentioned in header files,
grep '\.cd' include/*.h|grep '#include'|cut -f2 -d' '|sed -e 's/["<>]//g'|sort|uniq
