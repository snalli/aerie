#!/bin/bash
#

./build/src/sal/tool/pool/pool create -p /tmp/stamnos_pool -s 64M
./build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 32M -t mfs

exit 0 # ignore any failed commands 
