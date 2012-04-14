#!/bin/bash
#
# do cleanup

# Create the storage pool

./build/src/spa/tool/pool/pool create -p /tmp/stamnos_pool -s 64M
./build/src/cfs/tool/cfs create -p /tmp/stamnos_pool -s 32M -t cfs

exit 0 # ignore any failed commands 
