#!/bin/bash
#
# do cleanup

#TOOL='strace '
DEBUG_LEVEL=5
#UBENCH_NAME='ubench_cfs'; UBENCH_WD='/pxfs'
UBENCH_NAME='ubench_pxfs'; UBENCH_WD='/pxfs/test'
#UBENCH_NAME='ubench_vfs'; UBENCH_WD='/mnt/scmfs/test1/test2'
#UBENCH_NAME='ubench_vfs'; UBENCH_WD='/tmp/test'
#UBENCH_CMD='+fs_create -p /pxfs -n 1024 -s 200000 +fs_read -p /pxfs -n 1024 -s 200000'
#UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 16384 +fs_open -p $UBENCH_WD -n 1024"
#UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 512 +fs_read -p $UBENCH_WD -n 1024 -s 512"
#UBENCH_CMD="+fs_read -p $UBENCH_WD -n 1024 -s 512"
UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 512"
#UBENCH_CMD="+fs_open -p $UBENCH_WD -n 1024"
#UBENCH_NAME='ubench_osd'
#UBENCH_CMD='+hlock -o -c -n 16384'
#UBENCH_CMD=$*

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

$TOOL ./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool

exit

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
sleep 2
#gnome-terminal --geometry=140x25+0-100 -x bash -c "$TOOL./build/bench/ubench/$UBENCH_NAME -h 10000 -d $DEBUG_LEVEL $UBENCH_CMD; $WAITKEY"
#pkill -9 fsclient
#pkill -9 fsserver

#./build/bench/ubench/$UBENCH_NAME -h 10000 -d $DEBUG_LEVEL $UBENCH_CMD
# PXFS/RXFS
#./build/bench/ubench/ubench_pxfs -h 10000 -d 0 +fs_create -p /pxfs/x/xxxxx/xxxxxxxxxxxxxxxxxxxxxxx -n 10000 -s 16384
#./build/bench/ubench/ubench_rxfs -h 10000 -d 0 +fs_fread -p /rxfs/x/xxxxx/xxxxxxxxxxxxxxxxxxxxxxx -n 10000 -s 16384
#./build/bench/ubench/ubench_pxfs -h 10000 -d 5 +fs_read -p /pxfs/x/xxxxx/xxxxxxxxxxxxxxxxxxxxxxx -n 1024 -s 512
#./build/bench/ubench/ubench_pxfs -h 10000 -d 5 +fs_create -p /pxfs/ -n 1024 -s 512
#./build/bench/ubench/ubench_rxfs -h 10000 -d 5 +fs_fread -p /rxfs/ -n 1024 -s 512

# VFS
#./build/bench/ubench/ubench_vfs -h 10000 -d 5 +fs_create -p /mnt/scmfs/ -n 1024 -s 1048576
#./build/bench/ubench/ubench_vfs -h 10000 -d 5 +fs_read -p /mnt/scmfs/ -n 1024 -s 16384
exit 0 # ignore any failed commands 
