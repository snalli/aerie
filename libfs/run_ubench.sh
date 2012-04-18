#!/bin/bash
#
# do cleanup

DEBUG_LEVEL=0
UBENCH_CMD='+hlock -o -n 16384'
#UBENCH_CMD=$*

# Create the storage pool
./build/src/spa/tool/pool/pool create -p /tmp/stamnos_pool -s 64M
./build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 32M -t mfs

if [ "$1" = "-d" ]
then
TOOL='gdb --args '
else
WAITKEY='read'
fi
if [ "$1" = "-v" ]
then
TOOL='valgrind --tool=callgrind --collect-atstart=no '
else
WAITKEY='read'
fi
#pkill -9 fsclient
#pkill -9 fsserver
gnome-terminal --geometry=140x15+0+0 --tab -e "./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool" &
usleep 300
gnome-terminal --geometry=140x25+0-100 -x bash -c "$TOOL./build/ubench/osd/ubench -h 10000 -d $DEBUG_LEVEL $UBENCH_CMD; $WAITKEY"
#pkill -9 fsclient
#pkill -9 fsserver

exit 0 # ignore any failed commands 
