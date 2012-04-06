#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include "pxfs/client/libfs.h"
#include "pxfs/client/inode.h"
#include "pxfs/client/namespace.h"


pthread_attr_t attr;


int
main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	int          debug_level = 0;
	char         operation[16];
	char         ch = 0;
	char*        xdst;
	unsigned int nblocks;

	while ((ch = getopt(argc, argv, "d:h:li:o:n:"))!=-1) {
		switch (ch) {
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'h':
				xdst = optarg;
				break;
			case 'l':
				assert(setenv("RPC_LOSSY", "5", 1) == 0);
				break;
			case 'o':
				strcpy(operation, optarg); 
				break;
			case 'n':
				nblocks = atoi(optarg);
				break;
			default:
				break;
		}
	}

	pthread_attr_init(&attr);
	// set stack size to 32K, so we don't run out of memory
	pthread_attr_setstacksize(&attr, 32*1024);
	
	libfs_init(argc, argv);

	libfs_shutdown();

	return 0;
}
