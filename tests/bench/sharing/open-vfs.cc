#include "bench/sharing/barrier.h"
#include "ubench/time.h"
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int Writer(int /*debug_level*/, const char* /*xdst*/, int numops, int /*size*/)
{
    MEASURE_TIME_PREAMBLE
    unsigned long long runtime;
    hrtime_t runtime_cycles = 0;
    int fd;
    char* buf = (char*) malloc(4096 * 1024);

    BarrierWait();

    fd = open("/mnt/scmfs/dir/file1", O_RDWR);
    assert(fd > 0);
    MEASURE_TIME_START
    MEASURE_CYCLES_START
    for (int i = 0; i < numops; i++)
    {
        if (pwrite(fd, buf, 4096, 0) < 0)
            assert(0);
    }
    MEASURE_CYCLES_STOP
    ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
    MEASURE_TIME_STOP
    MEASURE_TIME_DIFF_USEC(runtime)
    std::cout << "WRITER\n" << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;

    close(fd);
    return 0;
}

int Reader(int /*debug_level*/, const char* /*xdst*/, int numops, int /*size*/)
{
    MEASURE_TIME_PREAMBLE
    int fd;
    char* buf = (char*) malloc(4096 * 1024);
    unsigned long long runtime;
    hrtime_t runtime_cycles = 0;

    BarrierWait();

    fd = open("/mnt/scmfs/dir/file1", O_RDONLY);
    assert(fd > 0);
    MEASURE_TIME_START
    MEASURE_CYCLES_START
    for (int i = 0; i < numops; i++)
    {
        if (pread(fd, buf, 4096, 0) < 0)
            assert(0);
    }
    MEASURE_CYCLES_STOP
    ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
    MEASURE_TIME_STOP
    MEASURE_TIME_DIFF_USEC(runtime)
    std::cout << "READER\n" << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
    close(fd);
    return 0;
}
