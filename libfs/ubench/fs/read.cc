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
__ubench_fs_read(const char* root, int numops, size_t size)
{
	MEASURE_TIME_PREAMBLE
	int                    ret = 0;
	unsigned long long     runtime;
	hrtime_t               runtime_cycles = 0;
	unsigned long long     sync_runtime;
	hrtime_t               sync_runtime_cycles;
	int                    fd;
	std::string**          path = new std::string*[numops];
	void*                  buf = new char[size];

	for (int i=0; i<numops; i++) {
		std::stringstream  ss;
		ss << std::string(root);
		ss << "/test-" << i << ".dat";
		path[i] = new std::string(ss.str());
	}
	
	for (int i=0; i<32; i++) {
		fd = fs_open(path[i]->c_str(), O_RDWR);
		assert(fd>0);
		fs_read(fd, buf, size);
		fs_close(fd);
	}


	MEASURE_TIME_START

	for (int i=32; i<numops; i++) {
		fd = fs_open(path[i]->c_str(), O_RDWR);
		assert(fd>0);
    	MEASURE_CYCLES_START
		fs_read(fd, buf, size);
    	MEASURE_CYCLES_STOP
		ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
		fs_close(fd);
	}

	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    //MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	std::cout << "READ\n" << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	return ret;
}


int
ubench_fs_read(int argc, char* argv[])
{
	extern int  optind;
	extern int  opterr;
	char        ch;
	int         numops = 0;
	char*       objtype;
	const char* root_path = NULL;
	size_t      size = 0;
	
	opterr=0;
	optind=0;
	while ((ch = getopt(argc, argv, "p:n:s:"))!=-1) {
		switch (ch) {
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
			default:
				break;
		}
	}

	if (!root_path) {
		return -1;
	}

	return __ubench_fs_read(root_path, numops, size);
}
