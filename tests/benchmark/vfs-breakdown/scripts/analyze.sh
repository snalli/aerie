#!/bin/bash

. common.sh

function analyze
{
	local PERF_DATA_PREFIX=$1
	echo 'REPORT: '${PERF_DATA_PREFIX}
	${PERF} report -i ${AERIE_ROOT}/libfs/${PERF_DATA_PREFIX}.perf.data -n > ${AERIE_ROOT}/libfs/perf.data.out
	python ${ANALYZE_PERF} ${AERIE_ROOT}/libfs/perf.data.out ${ANALYZE_ROOT}/modules ${ANALYZE_ROOT}/linux-tags
}

#analyze stat
#analyze open
#analyze rename
#analyze unlink
#analyze read
#analyze write
analyze create
