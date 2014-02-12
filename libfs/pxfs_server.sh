#!/bin/sh

#read name
#echo $name
#echo 0 > /sys/devices/system/cpu/cpu1/online 
#echo 0 > /sys/devices/system/cpu/cpu2/online 
#echo 0 > /sys/devices/system/cpu/cpu3/online 

echo 3 >> /proc/sys/vm/drop_caches
echo > debug
echo 0 > /proc/sys/kernel/randomize_va_space 
pkill -9 fsserver
pkill -9 operf
pkill -9 filebench
pkill -9 filereader
rm -fr /tmp/shbuf_*
#/scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 16384M 
/scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 8192M
#/scratch/nvm/stamnos/libfs/build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 16384M -t mfs
#/scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool & 

#operf /scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool & 

#strace /scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 8192M
#strace /scratch/nvm/stamnos/libfs/build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 12G -t mfs
#strace /scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool &

#gdb --args /scratch/nvm/stamnos/libfs/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s 1024M
#gdb --args /scratch/nvm/stamnos/libfs/build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s 12G -t mfs
#gdb --args /scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool 

#valgrind  --track-origins=yes /scratch/nvm/stamnos/libfs/build/src/pxfs/server/fsserver -p 10000 -d 0 -s /tmp/stamnos_pool >output.txt 2>&1  



