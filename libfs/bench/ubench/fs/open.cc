#include "ubench/fs/fs.h"
#include "ubench/time.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>

static int usage()
{
    return -1;
}

static int __ubench_fs_open(const char* root, int numops)
{
    MEASURE_TIME_PREAMBLE
    int ret = 0;
    int fd = 0;
    unsigned long long runtime;
    hrtime_t runtime_cycles = 0;
    std::string** path = new std::string*[numops];

    for (int i = 0; i < numops; i++)
    {
        std::stringstream ss;
        ss << std::string(root);
        ss << "/test-" << i << ".dat";
        path[i] = new std::string(ss.str());
    }

    MEASURE_TIME_START

    for (int i = 0; i < numops; i++)
    {
        MEASURE_CYCLES_START
        fd = fs_open(path[i]->c_str(), O_RDWR);
        MEASURE_CYCLES_STOP
        assert(fd > 0);
        ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
        fs_close(fd);
    }

    MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)

    std::cout << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
    return ret;
}

int ubench_fs_open(int argc, char* argv[])
{
    extern int optind;
    extern int opterr;
    char ch;
    int numops = 0;
    const char* root_path = NULL;

    opterr = 0;
    optind = 0;
    while ((ch = getopt(argc, argv, "p:n:")) != -1)
    {
        switch (ch)
        {
        case 'p': // root path
            root_path = optarg;
            /* fall through */
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

    if (!root_path)
    {
        return -1;
    }

    return __ubench_fs_open(root_path, numops);
}
