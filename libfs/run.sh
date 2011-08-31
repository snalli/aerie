#!/bin/bash
if [ "$1" = "-r" ]
then
rm chunkstore.locks
rm *.pheap
fi
pkill -9 fsclient
pkill -9 fsserver
./build/src/server/fsserver -p 10000
#./build/src/server/fsserver -p 10000 &
#gdb ./build/src/server/fsserver
#./build/src/client/fsclient -p 10000 -i 1 &
#./build/src/client/fsclient -p 10000 -i 2 -o mkfs
#./build/src/client/fsclient -p 10000 -i 2 -o demo
#pkill -9 fsclient
#pkill -9 fsserver
