#!/bin/bash
#
# do cleanup

# Create the storage pool

./build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 1024M
./build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 512M -t mfs

exit 0 # ignore any failed commands 
