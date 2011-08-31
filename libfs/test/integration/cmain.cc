#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <iostream>
#include "rpc/rpc.h"
#include "rpc/jsl_log.h"
#include "client/libfs.h"
#include "tool/testfw/integrationtest.h"


int
main(int argc, char *argv[])
{
	pthread_attr_t attr;
	int            ret;
	int            debug_level = 0;
	uid_t          principal_id;
	char           ch = 0;
	char*          suiteName;
	char*          testName;
	char*          xdst=NULL;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	principal_id = getuid();


	while ((ch = getopt(argc, argv, "d:h:li:t:s:"))!=-1) {
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

			// don't use 't' and 's' because they are already used by the
			// testing framework
			case 's':
			case 't':
			default:
				break;
		}
	}

	if (!xdst) {
		return -1;
	}

	if (debug_level > 0) {
		dbg_set_level(debug_level);
		jsl_set_debug(debug_level);
		jsl_log(JSL_DBG_1, "DEBUG LEVEL: %d\n", debug_level);
	}

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);
	
	getTest(argc, argv, &suiteName, &testName);
	libfs_init(principal_id, xdst);
	ret = runTests(suiteName, testName);
	libfs_shutdown();
	return ret;
}
