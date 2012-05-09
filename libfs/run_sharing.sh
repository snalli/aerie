#!/bin/bash
#
# do cleanup

#TOOL='strace '
DEBUG_LEVEL=0
NUMOPS=1048576
SIZE=4096
#SCHED0='taskset 0x1'
#SCHED1='taskset 0x2'
#SCHED2='taskset 0x4'
#SCHED3='taskset 0x8'
#SCHED4='taskset 0x16'
#SCHED5='taskset 0x32'

# Create the storage pool
#./build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 512M
#./build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 256M -t mfs

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

pkill -9 sharing_pxfs
pkill -9 fsserver


# Start Server
#gnome-terminal --geometry=140x15+0+0 --tab -e "gdb --args ./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool" &
#gnome-terminal --geometry=140x15+0+0 --tab -e "$SCHED0 ./build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool" &

sleep 2
# Start Client
./build/bench/sharing/sharing_vfs -h 10000 -d 5 -b create -c 5

gnome-terminal --geometry=140x25+0-100 -x bash -c "$SCHED1 $TOOL./build/bench/sharing/sharing_vfs -h 10000 -d $DEBUG_LEVEL -n $NUMOPS -s $SIZE -b writer; $WAITKEY" &
gnome-terminal --geometry=140x25+0-100 -x bash -c "$SCHED2 $TOOL./build/bench/sharing/sharing_vfs -h 10000 -d $DEBUG_LEVEL -n $NUMOPS -s $SIZE -b writer; $WAITKEY" &
gnome-terminal --geometry=140x25+0-100 -x bash -c "$SCHED2 $TOOL./build/bench/sharing/sharing_vfs -h 10000 -d $DEBUG_LEVEL -n $NUMOPS -s $SIZE -b writer; $WAITKEY" &
gnome-terminal --geometry=140x25+0-100 -x bash -c "$SCHED3 $TOOL./build/bench/sharing/sharing_vfs -h 10000 -d $DEBUG_LEVEL -n $NUMOPS -s $SIZE -b writer; $WAITKEY" &
gnome-terminal --geometry=140x25+0-100 -x bash -c "$SCHED4 $TOOL./build/bench/sharing/sharing_vfs -h 10000 -d $DEBUG_LEVEL -n $NUMOPS -s $SIZE -b writer; $WAITKEY" &
#gnome-terminal --geometry=140x25+0-100 -x bash -c "$SCHED5 $TOOL./build/bench/sharing/sharing_vfs -h 10000 -d $DEBUG_LEVEL -n $NUMOPS -s $SIZE -b writer; $WAITKEY" &
#pkill -9 fsclient
#pkill -9 fsserver

# PXFS/RXFS
exit 0 # ignore any failed commands 
