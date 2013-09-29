#!/bin/bash
#
# do cleanup

#TOOL='strace '
DEBUG_LEVEL=0
#UBENCH_NAME='ubench_pxfs'; UBENCH_WD='/pxfs'
#UBENCH_NAME='ubench_vfs'; UBENCH_WD='/mnt/scmfs'
UBENCH_NAME='ubench_vfs'; UBENCH_WD='/tmp'

## Microbenchmarks to run
#UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 4096"
#UBENCH_CMD="+fs_delete -p $UBENCH_WD -n 1024 -s 4096"
#UBENCH_CMD="+fs_seqread -p $UBENCH_WD -n 32 -s 4096"
#UBENCH_CMD="+fs_randread -p $UBENCH_WD -n 25600 -s 4096"
#UBENCH_CMD="+fs_seqwrite -p $UBENCH_WD -n 32 -s 4096"
#UBENCH_CMD="+fs_randwrite -p $UBENCH_WD -n 25600 -s 4096"
#UBENCH_CMD="+fs_append -p $UBENCH_WD -n 1 -s 4096"

## Ignore the following UBENCH_CMD
#UBENCH_CMD='+fs_create -p /pxfs -n 1024 -s 200000 +fs_read -p /pxfs -n 1024 -s 200000'
#UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 16384 +fs_open -p $UBENCH_WD -n 1024"
#UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 512 +fs_read -p $UBENCH_WD -n 1024 -s 512"
UBENCH_CMD="+fs_create -p $UBENCH_WD -n 1024 -s 512 +fs_open -p $UBENCH_WD -n 1024"

#UBENCH_NAME='ubench_osd'
#UBENCH_CMD='+hlock -o -c -n 16384'
#UBENCH_CMD=$*

# Create the storage pool
if [ "$UBENCH_NAME" = "ubench_pxfs" ]
then
./build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 512M
./build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 256M -t mfs
fi

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

pkill -9 fsclient
pkill -9 fsserver

# Start Server
if [ "$UBENCH_NAME" = "ubench_pxfs" ]
then
#gnome-terminal --geometry=140x15+0+0 --tab -e "$TOOL./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool" &
echo 'Running PXFS Server'
./build/src/pxfs/server/fsserver -p 10000 -d $DEBUG_LEVEL -s /tmp/stamnos_pool &
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

CMD="$TOOL./build/bench/ubench/$UBENCH_NAME -h 10000 -d $DEBUG_LEVEL $UBENCH_CMD"
echo $CMD
$CMD
exit 0 # ignore any failed commands 
