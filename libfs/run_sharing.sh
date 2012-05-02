#!/bin/bash
#
# do cleanup

#TOOL='strace '
DEBUG_LEVEL=5

# Create the storage pool
./build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 512M
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
gnome-terminal --geometry=140x15+0+0 --tab -e "$TOOL./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool" &

# Start Client
sleep 2
./build/bench/sharing/sharing_pxfs -h 10000 -d 5 -b create 

gnome-terminal --geometry=140x25+0-100 -x bash -c "$TOOL./build/bench/sharing/sharing_pxfs -h 10000 -d $DEBUG_LEVEL -b open; $WAITKEY" &
sleep 1
gnome-terminal --geometry=140x25+0-100 -x bash -c "$TOOL./build/bench/sharing/sharing_pxfs -h 10000 -d $DEBUG_LEVEL -b open; $WAITKEY"
#pkill -9 fsclient
#pkill -9 fsserver

# PXFS/RXFS
exit 0 # ignore any failed commands 
