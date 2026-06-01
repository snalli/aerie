#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "time.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <iostream>
# include <string.h>
# include <sched.h>
# include <math.h>

struct timeval start, end;

void print_time_diff()
{
        double st = 0, et = 0;

        et = end.tv_sec;
        et += (end.tv_usec * pow(10,-6));
        st = start.tv_sec;
        st += (start.tv_usec * pow(10,-6));
	printf("time diff : %lf\n", et-st);
	return;
}

void start_timer()
{
	gettimeofday(&start, NULL);
}

void end_timer()
{
	gettimeofday(&end, NULL);
}
int main(int argc, char **argv)
{
	MEASURE_TIME_PREAMBLE
	struct stat buf;
	unsigned long files = 1024UL*1024UL, i;
	char file[32];
	int fd;
	unsigned long long     runtime;
        hrtime_t               runtime_cycles = 0;
        unsigned long long     sync_runtime;
        hrtime_t               sync_runtime_cycles;

	if(argc > 1)
		files = (unsigned long)atoi(argv[1]);

	for(i = 0;i < files;i++)
	{
		sprintf(file, "/tmp/stattest/%d.tmp", i);
		fd = open(file, O_CREAT | O_RDWR);
		if(fd < 0)
		{
			printf("file creation error");
			exit(0);
		}
		close(fd);
	}

	sync();
	//MEASURE_TIME_START

	for(i = 0;i < files;i++)
	{
		sprintf(file, "/tmp/stattest/%d.tmp", i);
		fd = open(file, O_RDWR);
		if(fd < 0)
		{
			printf("file could not be opened");
			exit(0);
		}
		MEASURE_CYCLES_START
		//start_timer();
		//fd = stat(file, &buf);
		if(fstat(fd, &buf) < 0)
		{
			printf("stat failed\n");
			exit(0);
		}
		//end_timer();
		//print_time_diff();
		MEASURE_CYCLES_STOP
		ADD_MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
		if(fd < 0)
		{
			printf("stat error");
			exit(0);
		}
	}
	MEASURE_TIME_STOP

	MEASURE_TIME_DIFF_USEC(runtime)
        std::cout << "STAT\n" << measure_time_summary(files, runtime, runtime_cycles) << std::endl;

	return 0;	
}
