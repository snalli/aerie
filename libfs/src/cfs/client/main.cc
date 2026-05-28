#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include "cfs/client/libfs.h"


pthread_attr_t attr;


int
main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	char         operation[16];
	char         ch = 0;

	while ((ch = getopt(argc, argv, "d:h:li:o:n:"))!=-1) {
		switch (ch) {
			case 'l':
				assert(setenv("RPC_LOSSY", "5", 1) == 0);
				break;
			case 'o':
				strcpy(operation, optarg);
				break;
			default:
				break;
		}
	}

	pthread_attr_init(&attr);
	// set stack size to 32K, so we don't run out of memory
	pthread_attr_setstacksize(&attr, 32*1024);
	
	cfs_init(argc, argv);

	cfs_shutdown();

	return 0;
}
