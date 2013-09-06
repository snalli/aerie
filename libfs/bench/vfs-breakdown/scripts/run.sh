#!/bin/bash

AERIE_ROOT=/home/volos/workspace/aerie
PERFREMOTE=${AERIE_ROOT}/libfs/build/tool/perfremote/perfremote
PERF=${AERIE_ROOT}/kernelmode/linux-3.9/tools/perf/perf
UBENCH_ROOT=${AERIE_ROOT}/libfs/build/bench/vfs-breakdown
FILESET_ROOT=/home/volos/tmp/bigfileset
ANALYZE_ROOT=${AERIE_ROOT}/libfs/tool/breakdown/perf
ANALYZE_PERF=${ANALYZE_ROOT}/analyze.py

function run_and_analyze
{
	CMD=$1

	echo ${CMD}
	rm ${AERIE_ROOT}/libfs/perf.data
	rm ${AERIE_ROOT}/libfs/perf.data.out
	${CMD} -m
	${PERF} report -n > ${AERIE_ROOT}/libfs/perf.data.out
	python ${ANALYZE_PERF} ${AERIE_ROOT}/libfs/perf.data.out ${ANALYZE_ROOT}/modules ${ANALYZE_ROOT}/linux-tags
}


#change working directory to help dynamic linker find libraries 
cd ${AERIE_ROOT}/libfs

pkill -9 perfremote
pkill -9 perf

${PERFREMOTE} -p ${PERF} &

rm -rf ${FILESET_ROOT}
CMD="${UBENCH_ROOT}/create -p ${FILESET_ROOT} -f 100000 -i 20 -l 1000"
${CMD}

CMD="${UBENCH_ROOT}/open -p ${FILESET_ROOT} -f 100000 -n 100000 -i 20 -l 1000"
run_and_analyze "${CMD}"
