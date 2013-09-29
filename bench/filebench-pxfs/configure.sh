#!/bin/bash 
ROOTDIR=`pwd`/../..
export CPPFLAGS="-I${ROOTDIR} -D_SCM_POOL_KERNELMODE -D_CLT2SVR_RPCNET -D_SVR2CLT_RPCNET -D__STAMNOS_EXPAND_DEBUG -DLIBFS"
export LIBS="-Wl,-rpath=${ROOTDIR}/libfs  \
${ROOTDIR}/libfs/build/src/pxfs/client/libfsc.so \
${ROOTDIR}/libfs/build/src/rxfs/client/libfsc.so \
${ROOTDIR}/libfs/build/src/pxfs/mfs/libmfsclt.so \
${ROOTDIR}/libfs/build/src/osd/libosdclt.so \
${ROOTDIR}/libfs/build/src/scm/libscm.so \
${ROOTDIR}/libfs/build/src/common/libcommon.so \
${ROOTDIR}/libfs/build/src/bcs/backend/rpc-net/librpc.so \
${ROOTDIR}/libfs/build/src/bcs/backend/rpc-fast/libfastrpc.so \
${ROOTDIR}/libfs/build/src/bcs/libbcsclt.so \
${ROOTDIR}/libfs/build/src/kvfs/client/libkvfsc.so"
./configure
