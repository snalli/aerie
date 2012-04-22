#!/bin/bash
#
# do cleanup

DEBUG_LEVEL=5
UBENCH_NAME='ubench_pxfs'
UBENCH_CMD='+fs_create -p /pxfs -n 16 -s 1024'
#UBENCH_CMD='+fs_create -p /pxfs -n 16 +fs_unlink -p /pxfs/ -n 16'
#UBENCH_NAME='ubench_osd'
#UBENCH_CMD='+hlock -o -c -n 16384'
#UBENCH_CMD=$*

# Create the storage pool
./build/src/spa/tool/pool/pool create -p /tmp/stamnos_pool -s 512M
./build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 256M -t mfs

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

# Start Server
if [ "$UBENCH_NAME" = "ubench_pxfs" ]
then
gnome-terminal --geometry=140x15+0+0 --tab -e "$TOOL./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool" &
fi
if [ "$UBENCH_NAME" = "ubench_cfs" ]
then
gnome-terminal --geometry=140x15+0+0 --tab -e "$TOOL./build/src/cfs/server/cfsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool" &
fi

# Start Client
usleep 300
gnome-terminal --geometry=140x25+0-100 -x bash -c "$TOOL./build/ubench/$UBENCH_NAME -h 10000 -d $DEBUG_LEVEL $UBENCH_CMD; $WAITKEY"
#pkill -9 fsclient
#pkill -9 fsserver

exit 0 # ignore any failed commands 
