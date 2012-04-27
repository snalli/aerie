#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <sstream>
#include <errno.h>
#include "ubench/fs/fs.h"
#include "ubench/time.h"

static int
usage() 
{
	return -1;
}


static int 
__ubench_fs_unlink(const char* root, int numops)
{
	MEASURE_TIME_PREAMBLE
	int                    ret = 0;
	int                    fd = 0;
	unsigned long long     runtime;
	hrtime_t               runtime_cycles;
	unsigned long long     sync_runtime;
	hrtime_t               sync_runtime_cycles;
	std::string**          path = new std::string*[numops];

	for (int i=0; i<numops; i++) {
		std::stringstream  ss;
		ss << std::string(root);
		ss << "/test-" << i << ".dat";
		path[i] = new std::string(ss.str());
	}


	MEASURE_TIME_START

	for (int i=0; i<numops; i++) {
		ret = fs_unlink(path[i]->c_str());
		assert(ret == 0);
	}

	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	MEASURE_TIME_START
	fs_sync();
	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(sync_runtime)
    MEASURE_TIME_DIFF_CYCLES(sync_runtime_cycles)

	std::cout << "UNLINK:\n" << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	std::cout << "SYNC:\n" << measure_time_summary(1, sync_runtime, sync_runtime_cycles) << std::endl;
	return ret;
}


int
ubench_fs_unlink(int argc, char* argv[])
{
	extern int  optind;
	extern int  opterr;
	char        ch;
	int         numops = 0;
	char*       objtype;
	const char* root_path = NULL;
	
	opterr=0;
	optind=0;
	while ((ch = getopt(argc, argv, "p:n:"))!=-1) {
		switch (ch) {
			case 'p': // root path
				root_path = optarg;
			case 'n':
				numops = atoi(optarg);
				break;
			case '?':
				usage();
			default:
				break;
		}
	}
	
	if (!root_path) {
		return -1;
	}

	return __ubench_fs_unlink(root_path, numops);
}
