#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include "rpc/rpc.h"
#include "rpc/jsl_log.h"
#include "server/api.h"
#include "client/libfs.h"
#include "client/inode.h"
#include "client/namespace.h"


pthread_attr_t attr;
unsigned int principal_id;


int
main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
    int   port;
	int   debug_level = 0;
	uid_t principal_id;
	char  operation[16];
	char  ch = 0;
	char* xdst;

	principal_id = getuid();

	while ((ch = getopt(argc, argv, "d:h:li:o:s:"))!=-1) {
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
			case 'i':
				principal_id = atoi(optarg);
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
	
	dbg_init(debug_level, NULL);

	libfs_init(principal_id, xdst);

	if (strcmp(operation, "mkfs") == 0) {
		libfs_mkfs("/superblock/A", "mfs", 0);
	} else {
		// unknown
	}

	libfs_shutdown();

	return 0;
}
