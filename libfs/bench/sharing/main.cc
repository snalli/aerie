#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <vector>
#include <getopt.h>
#include <iostream>
#include <sched.h>
#include "bench/sharing/barrier.h"

int Create(int debug_level, const char* xdst);
int Writer(int debug_level, const char* xdst, int numops, int size);
int Reader(int debug_level, const char* xdst, int numops, int size);


int
main(int argc, char *argv[])
{
	pthread_attr_t     attr;
	char               ch = 0;
	int                ret = -1;
	int                debug_level = -1;
	const char*        xdst="10000";
	const char*        ubench = NULL;
	extern int         opterr;
	extern char*       optarg;
	int                num_clients=0;
	int                num_ops=0;
	int                size=0;


	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	opterr=0;
	while ((ch = getopt(argc, argv, "d:h:b:c:n:"))!=-1) {
		switch (ch) {
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'h':
				xdst = optarg;
				break;
			case 'b':
				ubench = optarg;
				break;
			case 'c':
				num_clients = atoi(optarg);
				break;
			case 'n':
				num_ops = atoi(optarg);
				break;
			case 's':
				size = atoi(optarg);
				break;
			default:
				break;
		}
	}

	if (ubench == NULL) {
		return -1;
	}

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);
	
	if (strcmp(ubench, "create") == 0) {
		BarrierInit(num_clients);
		return Create(debug_level, xdst);
	}
	if (strcmp(ubench, "writer") == 0) {
		printf("WRITER\n");
		return Writer(debug_level, xdst, num_ops, size);
	}
	if (strcmp(ubench, "reader") == 0) {
		printf("READER\n");
		return Reader(debug_level, xdst, num_ops, size);
	}

	return ret;
}
