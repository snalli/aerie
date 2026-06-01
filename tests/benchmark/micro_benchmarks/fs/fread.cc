#include "micro_benchmarks/fs/fs.h"
#include "micro_benchmarks/time.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <iostream>
#include <pthread.h>
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

struct Args
{
    const char* root;
    int numops;
    size_t size;
};

static void* __ubench_fs_fread(void* arg)
{
    MEASURE_TIME_PREAMBLE
    const char* root = ((Args*) arg)->root;
    int numops = ((Args*) arg)->numops;
    size_t size = ((Args*) arg)->size;
    unsigned long long runtime;
    hrtime_t runtime_cycles = 0;
    RFile* fp;
    std::string** path = new std::string*[numops];
    void* buf = new char[size];

    for (int i = 0; i < numops; i++)
    {
        std::stringstream ss;
        ss << std::string(root);
        ss << "/test-" << i << ".dat";
        path[i] = new std::string(ss.str());
    }

    for (int i = 0; i < 8; i++)
    {
        fp = fs_fopen(path[i]->c_str(), O_RDWR);
        assert(fp != NULL);
        fs_fread(fp, buf, size);
        fs_fclose(fp);
    }

    MEASURE_TIME_START

    for (int i = 8; i < numops; i++)
    {
        fp = fs_fopen(path[i]->c_str(), O_RDWR);
        assert(fp != NULL);
        MEASURE_CYCLES_START
        fs_fread(fp, buf, size);
        MEASURE_CYCLES_STOP
        ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
        fs_fclose(fp);
    }

    MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)

    std::cout << "READ\n" << measure_time_summary(numops - 8, runtime, runtime_cycles) << std::endl;
    return NULL;
}

int ubench_fs_fread(int argc, char* argv[])
{
    extern int optind;
    extern int opterr;
    char ch;
    int numops = 0;
    const char* root_path = NULL;
    size_t size = 0;

    opterr = 0;
    optind = 0;
    while ((ch = getopt(argc, argv, "p:n:s:")) != -1)
    {
        switch (ch)
        {
        case 'p': // root path
            root_path = optarg;
            break;
        case 'n':
            numops = atoi(optarg);
            break;
        case 's':
            size = atoi(optarg);
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

    struct Args args;
    args.root = root_path;
    args.numops = numops;
    args.size = size;

    pthread_t threads[8];
    int nthreads = 4;
    for (int i = 0; i < nthreads; i++)
    {
        pthread_create(&threads[i], NULL, __ubench_fs_fread, (void*) &args);
    }
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    return 0;
    //__ubench_fs_fread(&args);
}
