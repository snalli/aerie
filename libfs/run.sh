#!/bin/bash
if [ "$1" = "-r" ]
then
rm chunkstore.locks
rm *.pheap
fi
pkill -9 fsclient
pkill -9 fsserver
./server/fsserver -p 10000
#./server/fsserver -p 10000 &
#gdb ./server/fsserver
#./client/fsclient -p 10000 -i 1 &
#./client/fsclient -p 10000 -i 2 -o mkfs
#./client/fsclient -p 10000 -i 2 -o demo
#pkill -9 fsclient
#pkill -9 fsserver
