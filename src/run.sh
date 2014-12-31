#!/usr/bin/env sh
sudo \
LD_LIBRARY_PATH=`pwd` \
rtl_fm -f 102.4e6 -s 200000 -r 48000 - | \
aplay -r 48k -f S16_LE -t raw -c 1
