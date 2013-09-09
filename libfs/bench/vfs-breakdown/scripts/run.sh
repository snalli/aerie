#!/bin/bash

AERIE_ROOT=/home/volos/workspace/aerie
PERFREMOTE=${AERIE_ROOT}/libfs/build/tool/perfremote/perfremote
PERF=${AERIE_ROOT}/kernelmode/linux-3.9/tools/perf/perf
UBENCH_ROOT=${AERIE_ROOT}/libfs/build/bench/vfs-breakdown
FILESET_ROOT=/home/volos/tmp/bigfileset
ANALYZE_ROOT=${AERIE_ROOT}/libfs/tool/breakdown/perf
ANALYZE_PERF=${ANALYZE_ROOT}/analyze.py

FILESET_NFILES=1000000
FILESET_DIRWIDTH_INNER=20
FILESET_DIRWIDTH_LEAF=1000

NOPS=1000

function drop_dentry_inode_cache
{
	echo 2 | sudo tee /proc/sys/vm/drop_caches
}

function analyze
{
	python ${ANALYZE_PERF} ${AERIE_ROOT}/libfs/perf.data.out ${ANALYZE_ROOT}/modules ${ANALYZE_ROOT}/linux-tags
}

function monitor_and_analyze
{
	CMD=$1

	rm ${AERIE_ROOT}/libfs/perf.data
	rm ${AERIE_ROOT}/libfs/perf.data.out
	CMD="${CMD} -m"
	echo ${CMD} 
	${CMD}
	${PERF} report -n > ${AERIE_ROOT}/libfs/perf.data.out
	python ${ANALYZE_PERF} ${AERIE_ROOT}/libfs/perf.data.out ${ANALYZE_ROOT}/modules ${ANALYZE_ROOT}/linux-tags
}

function create_fileset
{
	local FILESIZE=$1

	echo "Creating fileset..."
	rm -rf ${FILESET_ROOT}
	CMD="${UBENCH_ROOT}/create -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -s ${FILESIZE}"
	echo ${CMD}
	${CMD}
}

function run_stat
{
	local NOPS=$1

	echo "Running stat..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS} -s"
	monitor_and_analyze "${CMD}"
}

function run_open
{
	local NOPS=$1

	echo "Running open..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS}"
	monitor_and_analyze "${CMD}"
}

function run_read
{
	local NOPS=$1
	local FILESIZE=64

	echo "Running read..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS} -r ${FILESIZE}"
	monitor_and_analyze "${CMD}"
}

function run_write
{
	local NOPS=$1
	local FILESIZE=64

	echo "Running write..."
	CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS} -w ${FILESIZE}"
	monitor_and_analyze "${CMD}"
}

function run_unlink
{
	local NOPS=$1

	echo "Running unlink..."
	CMD="${UBENCH_ROOT}/unlink -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS}"
	monitor_and_analyze "${CMD}"
}

function run_rename
{
	local NOPS=$1

	echo "Running rename..."
	CMD="${UBENCH_ROOT}/rename -p ${FILESET_ROOT} -f ${FILESET_NFILES} -i ${FILESET_DIRWIDTH_INNER} -l ${FILESET_DIRWIDTH_LEAF} -n ${NOPS}"
	monitor_and_analyze "${CMD}"
}


#analyze
#exit

# some setup
# -change working directory to aid dynamic linker find libraries 
# -kill any previous running perf processes
cd ${AERIE_ROOT}/libfs
pkill -9 perfremote
pkill -9 perf

# run perfremote so as to profile processes
${PERFREMOTE} -p ${PERF} &


#create_fileset 64
#drop_dentry_inode_cache
#run_stat ${NOPS}
#drop_dentry_inode_cache
#run_open ${NOPS}
#drop_dentry_inode_cache
#run_read ${NOPS}
#drop_dentry_inode_cache
#run_unlink ${NOPS}
run_rename ${NOPS}
