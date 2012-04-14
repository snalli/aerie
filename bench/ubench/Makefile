LIB = -lrt -lpthread \
    /home/hvolos/workspace/muses/stamnos/libfs/build/src/pxfs/client/libfsc.so \
	/home/hvolos/workspace/muses/stamnos/libfs/build/src/pxfs/mfs/libmfsclt.so \
	/home/hvolos/workspace/muses/stamnos/libfs/build/src/ssa/libssaclt.so \
	/home/hvolos/workspace/muses/stamnos/libfs/build/src/bcs/libbcsclt.so \
	/home/hvolos/workspace/muses/stamnos/libfs/build/src/spa/libspa.so \
	/home/hvolos/workspace/muses/stamnos/libfs/build/src/common/libcommon.so \
	/home/hvolos/workspace/gperftools-2.0/.libs/libprofiler.so \
#	/home/hvolos/workspace/hoard-38/src/libhoard.so

all:
	gcc open.c $(LIB) -o open
	gcc lopen.c $(LIB) -ldl -o lopen
