#!/bin/bash
#
# do cleanup

# Create the storage pool

./build/src/sal/tool/spool/spool --create /tmp/stamnos_spool 32M

exit 0 # ignore any failed commands 
