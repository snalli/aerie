#!/bin/sh

ROOTDIR=`pwd`
DEBUG=0
FS_SIZE_MB=1024 
POOL_SIZE_MB=`echo "${FS_SIZE_MB}+128" | bc -l`
SIGUSR1=10

if [ "$1" = "-s" ]
then
	pkill -${SIGUSR1} fsserver
elif [ "$1" = "-k" ]
then
	pkill -9 fsserver
else
	pkill -9 fsserver
	${ROOTDIR}/build/src/scm/tool/pool/pool create -p /tmp/stamnos_pool -s ${POOL_SIZE_MB}M
	${ROOTDIR}/build/src/pxfs/tool/pxfs create -p /tmp/stamnos_pool -s ${FS_SIZE_MB}M -t mfs
	if [ "$1" = "-d" ]
	then
		gdb --args ${ROOTDIR}/build/src/pxfs/server/fsserver -p 10000 -d 5 -s /tmp/stamnos_pool
	else 
		${ROOTDIR}/build/src/pxfs/server/fsserver -p 10000 -d 5 -s /tmp/stamnos_pool &
		FS_SERVER_PID=$!
		echo "FS_SERVER_PID = ${FS_SERVER_PID}"
	fi
fi
