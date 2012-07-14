#!/bin/sh

pkill -9 fsserver
/scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 16384M
/scratch/nvm/stamnos/libfs/build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 16000M -t mfs
#/scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 4096M
#/scratch/nvm/stamnos/libfs/build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 4000M -t mfs
#gdb --args /scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool
/scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool > out_server
