#!/bin/bash

ROOT=/home/hvolos/workspace/stamnos/libfs/
#ROOT=../../../
DEBUG_LEVEL=5

cd $ROOT

if [ "$1" = "-r" ]
then
rm chunkstore.locks
rm *.pheap
fi
pkill -9 fsclient
pkill -9 fsserver
gnome-terminal --geometry=140x25+0+0 --tab -e "./build/src/server/fsserver -p 10000 -d $DEBUG_LEVEL" &
#./build/test/interactive/lock_tester/lock_tester -h 10000 -i 1 -d $DEBUG_LEVEL
#gnome-terminal --geometry=140x25+0-100 -x bash -c "gdb --args ./build/test/interactive/lock_tester/lock_tester -h 10000 -i 1 -d $DEBUG_LEVEL; read"
gnome-terminal --geometry=140x25+0-100 -x bash -c "./build/test/interactive/lock_tester/lock_tester -h 10000 -i 1 -d $DEBUG_LEVEL; read"
#pkill -9 fsclient
#pkill -9 fsserver
