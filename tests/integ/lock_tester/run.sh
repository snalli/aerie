#!/bin/bash

#ROOT=/home/hvolos/workspace/stamnos/libfs/
#ROOT=/home/hvolos/workspace/muses/stamnos/libfs/test/interactive/lock_tester
#ROOT=../../../

DEBUG_LEVEL=5

#cd $ROOT

if [ "$1" = "-r" ]
then
rm chunkstore.locks
rm *.pheap
fi
if [ "$1" = "-d" ]
then
GDB='gdb --args '
else
WAITKEY='read'
fi
pkill -9 fsclient
pkill -9 fsserver
gnome-terminal --geometry=140x15+0+0 --tab -e "./build/src/server/fsserver -p 10000 -d $DEBUG_LEVEL" &
#./build/test/interactive/lock_tester/lock_tester -h 10000 -i 1 -d $DEBUG_LEVEL
gnome-terminal --geometry=140x15+0-400 -x bash -c "$GDB./build/test/interactive/lock_tester/lock_tester -h 10000 -i 1 -t C1 -d $DEBUG_LEVEL; $WAITKEY" &
gnome-terminal --geometry=140x15+0-100 -x bash -c "$GDB./build/test/interactive/lock_tester/lock_tester -h 10000 -i 2 -t C2 -d $DEBUG_LEVEL; $WAITKEY"
#pkill -9 fsclient
#pkill -9 fsserver
