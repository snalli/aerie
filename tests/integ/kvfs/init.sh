#!/bin/bash
#
# do cleanup

# Create the storage pool

./build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 64M
./build/src/kvfs/tool/kvfs create -p /tmp/stamnos_pool -s 32M

exit 0 # ignore any failed commands 
