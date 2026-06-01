#include "ubench/osd/osd.h"
#include "ubench/time.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

using namespace client;

static int usage()
{
    return -1;
}

static int __ubench_lock(osd::client::OsdSession* /*session*/, int numops, bool cache)
{
    MEASURE_TIME_PREAMBLE
    int ret = E_SUCCESS;
    unsigned long long runtime;
    hrtime_t runtime_cycles;

    if (cache)
    {
        for (int i = 0; i < numops; i++)
        {
            lock_protocol::Mode unused;
            if ((ret = global_storage_system->lckmgr()->Acquire(
                     osd::cc::client::LockId(i + 16), lock_protocol::Mode::XL, 0, unused)) < 0)
            {
                return ret;
            }
            if ((ret = global_storage_system->lckmgr()->Release(osd::cc::client::LockId(i + 16))) <
                0)
            {
                return ret;
            }
        }
    }

    MEASURE_TIME_START
    CALLGRIND_TOGGLE_COLLECT

    for (int i = 0; i < numops; i++)
    {
        lock_protocol::Mode unused;
        if ((ret = global_storage_system->lckmgr()->Acquire(
                 osd::cc::client::LockId(i + 16), lock_protocol::Mode::XL, 0, unused)) < 0)
        {
            return ret;
        }
    }

    CALLGRIND_TOGGLE_COLLECT
    MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    MEASURE_TIME_DIFF_CYCLES(runtime_cycles)

    std::cout << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
    return ret;
}

int ubench_lock(int argc, char* argv[])
{
    extern int optind;
    extern int opterr;
    char ch;
    int numops = 0;
    bool cache = false;

    opterr = 0;
    optind = 0;
    while ((ch = getopt(argc, argv, "cn:")) != -1)
    {
        switch (ch)
        {
        case 'c': // use cache lock
            cache = true;
            break;
        case 'n':
            numops = atoi(optarg);
            break;
        case '?':
            usage();
            break;
        default:
            break;
        }
    }
    osd::client::OsdSession* session = new osd::client::OsdSession(global_storage_system);
    return __ubench_lock(session, numops, cache);
}
