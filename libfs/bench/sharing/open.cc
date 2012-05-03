#include <stdlib.h>
#include "pxfs/client/libfs.h"
#include "pxfs/client/client_i.h"
#include "osd/main/common/lock_protocol.h"
#include "ubench/time.h"
#include "bench/sharing/barrier.h"


int 
Writer(int debug_level, const char* xdst, int numops, int size)
{
	MEASURE_TIME_PREAMBLE
	unsigned long long     runtime;
	hrtime_t               runtime_cycles = 0;
	int ret;
	int fd;
	char* buf = (char*) malloc(4096*1024);
	lock_protocol::Mode unused;

	libfs_init3(xdst, debug_level);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);

	BarrierWait();

	fd = libfs_open("/pxfs/dir/file1", O_RDWR);
	assert(fd>0);
	MEASURE_TIME_START
	MEASURE_CYCLES_START
	for (int i=0; i<numops; i++) {
		libfs_pwrite(fd, buf, 4096, 0);
	}
	MEASURE_CYCLES_STOP
	ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	MEASURE_TIME_STOP
    MEASURE_TIME_DIFF_USEC(runtime)
	std::cout << "WRITER\n" << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;

	libfs_close(fd);
	libfs_shutdown();
	return 0;
}


int 
Reader(int debug_level, const char* xdst, int numops, int size)
{
	MEASURE_TIME_PREAMBLE
	int ret;
	int fd;
	char* buf = (char*) malloc(4096*1024);
	lock_protocol::Mode unused;
	unsigned long long     runtime;
	hrtime_t               runtime_cycles = 0;


	libfs_init3(xdst, debug_level);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);

	BarrierWait();

	fd = libfs_open("/pxfs/dir/file1", O_RDONLY);
	assert(fd>0);
	MEASURE_TIME_START
	MEASURE_CYCLES_START
	for (int i=0; i<numops; i++) {
		libfs_pread(fd, buf, 4096, 0);
	}
	MEASURE_CYCLES_STOP
	ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	MEASURE_TIME_STOP
    MEASURE_TIME_DIFF_USEC(runtime)
	std::cout << "READER\n" << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	libfs_close(fd);
	libfs_shutdown();
	return 0;
}
