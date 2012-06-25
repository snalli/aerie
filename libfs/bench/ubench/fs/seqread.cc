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
__ubench_fs_seqread(const char* root, int numops, int warmup_ops, size_t size)
{
	MEASURE_TIME_PREAMBLE
	int                    ret = 0;
	unsigned long long     runtime;
	hrtime_t               runtime_cycles = 0;
	unsigned long long     sync_runtime;
	hrtime_t               sync_runtime_cycles;
	int                    fd;
	void*                  buf = new char[size];
	unsigned long		totalsize=0, onegig = 1024*1024*1024;
	unsigned long		exp_nr_reads = 0, nr_reads = 0;

	std::stringstream  ss;
        ss << std::string(root);
        ss << "/test.dat";

	/* file creation */
	fd = fs_open2(ss.str().c_str(), O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);
        assert(fd>0);
	/* populate the file with one gig of data */
	while(totalsize < onegig)
	{	
        	ret = fs_write(fd, buf, size);
		assert(ret == size);
		totalsize += size;
	}
	fs_close(fd);
	fs_fsync(fd);
	fs_sync();
	system("echo 3 >> /proc/sys/vm/drop_caches");
	exp_nr_reads = totalsize/size;
	printf("file creation %s is done\n", ss.str().c_str());

	MEASURE_TIME_START

	for (int i=0; i<numops; i++) {
		nr_reads = 0;
		fd = fs_open(ss.str().c_str(), O_RDWR);
		assert(fd>0);
    	MEASURE_CYCLES_START
		while((ret = fs_read(fd, buf, size)) > 0) nr_reads++;
    	MEASURE_CYCLES_STOP
		assert(nr_reads==exp_nr_reads);
		ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
		fs_close(fd);
		printf("read %d done\n", i);
	}

	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    //MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	std::cout << "SEQREAD\n" << measure_time_summary(numops*exp_nr_reads, runtime, runtime_cycles) << std::endl;
	return ret;
}


int
ubench_fs_seqread(int argc, char* argv[])
{
	extern int  optind;
	extern int  opterr;
	char        ch;
	int         numops = 0;
	char*       objtype;
	const char* root_path = NULL;
	size_t      size = 0;
	int         warmup_ops = 0;
	
	opterr=0;
	optind=0;
	while ((ch = getopt(argc, argv, "p:n:s:w:"))!=-1) {
		switch (ch) {
			case 'p': // root path
				root_path = optarg;
				break;
			case 'n':
				numops = atoi(optarg);
				break;
			case 'w':
				warmup_ops = atoi(optarg);
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

	return __ubench_fs_seqread(root_path, numops, warmup_ops, size);
}
