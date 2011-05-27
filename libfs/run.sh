#!/bin/bash
rm chunkstore.locks
rm chunkstore.pheap
rm chunkstore.pagepheap
pkill -9 fsclient
pkill -9 fsserver
./server/fsserver -p 10000  &
#./client/fsclient -p 10000 -i 1 &
./client/fsclient -p 10000 -i 2
#pkill -9 fsclient
#pkill -9 fsserver
