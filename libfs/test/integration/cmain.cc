#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <getopt.h>
#include <iostream>
#include <list>
#include "rpc/rpc.h"
#include "rpc/jsl_log.h"
#include "client/libfs.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"

extern int InitializeTest(testfw::TestFramework&) __attribute__((weak));

namespace testfw {
	TestFramework* __testfwp;
}

int
main(int argc, char *argv[])
{
	pthread_attr_t attr;
	int               ret;
	int               debug_level = 0;
	uid_t             principal_id;
	char              ch = 0;
	char*             xdst=NULL;
	extern int        opterr;
	std::string       unused;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	principal_id = getuid();

	opterr=0;
	while ((ch = getopt(argc, argv, "d:h:li:T:"))!=-1) {
		switch (ch) {
			case 'T':
				/* test framework argument -- ignore */
				break;
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
			default:
				break;
		}
	}

	testfw::TestFramework test_fw(argc, argv);
	testfw::__testfwp = &test_fw;

	if (test_fw.ArgVal("init", unused)==0) {
		if (InitializeTest) {
			return InitializeTest(test_fw);
		}
		return 0;
	}

	dbg_init(debug_level, NULL);

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);
	
	ret = test_fw.RunTests();

	return ret;
}
