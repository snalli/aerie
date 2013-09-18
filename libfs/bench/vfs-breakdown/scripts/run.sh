#!/bin/bash

. common.sh

FILESET_NFILES=1000000
FILESET_DIRWIDTH_INNER=20
FILESET_DIRWIDTH_LEAF=1000

NOPS=1000

MONITOR=0

function drop_dentry_inode_cache
{
#	echo 2 | sudo tee /proc/sys/vm/drop_caches
	echo 2 > /proc/sys/vm/drop_caches
}

function run_and_monitor
{
	local CMD=$1
	local PERF_DATA_PREFIX=$2

	rm ${AERIE_ROOT}/libfs/perf.data
        if [ ${MONITOR} -ne 0 ]; then
	  CMD="${CMD} -m"
        fi
	echo ${CMD} 
	${CMD}
	mv ${AERIE_ROOT}/libfs/perf.data ${AERIE_ROOT}/libfs/${PERF_DATA_PREFIX}.perf.data
}

function clear_fileset
{
	echo "Clearing fileset..."
	rm -rf ${FILESET_ROOT}
}

function create_fileset
{
	local FILESIZE=$1

	echo "Creating fileset..."
	CMD="${UBENCH_ROOT}/create -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -s ${FILESIZE}"
	echo ${CMD}
	${CMD}
}

function clear_and_create_fileset
{
	local FILESIZE=$1

	echo "Clearing fileset..."
	rm -rf ${FILESET_ROOT}
	echo "Creating fileset..."
	CMD="${UBENCH_ROOT}/create -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -s ${FILESIZE}"
	echo ${CMD}
	${CMD}
}

function run_stat
{
	local NOPS=$1

	echo "Running stat..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS} -s"
	run_and_monitor "${CMD}" stat
}

function run_create
{
	local NOPS=$1

	echo "Running create..."
	CMD="${UBENCH_ROOT}/create -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -s 0 -n ${NOPS}"
	run_and_monitor "${CMD}" create
}

function run_open
{
	local NOPS=$1

	echo "Running open..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS}"
	run_and_monitor "${CMD}" open
}

function run_read
{
	local NOPS=$1
	local FILESIZE=64

	echo "Running read..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS} -r ${FILESIZE}"
	run_and_monitor "${CMD}" read
}

function run_write
{
	local NOPS=$1
	local FILESIZE=64

	echo "Running write..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS} -w ${FILESIZE}"
	run_and_monitor "${CMD}" write
}

function run_unlink
{
	local NOPS=$1

	echo "Running unlink..."
	CMD="${UBENCH_ROOT}/unlink -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS}"
	run_and_monitor "${CMD}" unlink
}

function run_rename
{
	local NOPS=$1

	echo "Running rename..."
	CMD="${UBENCH_ROOT}/rename -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS}"
	run_and_monitor "${CMD}" rename
}


#analyze
#exit

# some setup
# -change working directory to aid dynamic linker find libraries 
# -kill any previous running perf processes
cd ${AERIE_ROOT}/libfs
pkill -9 perfremote
pkill -9 perf

if [ ${MONITOR} -ne 0 ]; then
  ${PERFREMOTE} -p ${PERF} &
fi

clear_fileset
drop_dentry_inode_cache
run_create 1000000

exit

#clear_and_create_fileset 64
#drop_dentry_inode_cache
#run_read 150000

#exit

clear_and_create_fileset 0
drop_dentry_inode_cache
run_write 150000

exit

clear_and_create_fileset 0
drop_dentry_inode_cache
run_stat 1000000

clear_and_create_fileset 0
drop_dentry_inode_cache
run_open 1000000
#drop_dentry_inode_cache
#run_open 1000000

clear_and_create_fileset 0
drop_dentry_inode_cache
run_rename 100000
#drop_dentry_inode_cache
#run_rename 100000

clear_and_create_fileset 0
drop_dentry_inode_cache
run_unlink 100000
#drop_dentry_inode_cache
#run_unlink 100000

exit

clear_and_create_fileset 64
drop_dentry_inode_cache
run_read 100000

clear_and_create_fileset 0
drop_dentry_inode_cache
run_write 100000
