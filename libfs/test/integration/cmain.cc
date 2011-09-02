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

extern int InitializeTests(std::list<std::string>&) __attribute__((weak));

int
main(int argc, char *argv[])
{
	pthread_attr_t attr;
	int               ret;
	int               debug_level = 0;
	uid_t             principal_id;
	char              ch = 0;
	char*             suiteName;
	char*             testName;
	char*                   xdst=NULL;
	std::list<std::string>  test_args_list;


	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	principal_id = getuid();

	while ((ch = getopt(argc, argv, "T:d:h:li:t:s:"))!=-1) {
		switch (ch) {
			case 'T':
				// Test specific initilization parameters are collected
				// here and passed to InitializeTests
				{
					std::string str(optarg);
					size_t      p;
					size_t      np;

					if (optarg[0] == ',') {
						p = 1;
						for(;;) {
							np = str.find(',', p);
							if (np != std::string::npos) {
								//std::cout << str.substr(p, np-p) << std::endl;
								test_args_list.push_back(str.substr(p, np-p));
							} else {
								test_args_list.push_back(str.substr(p));
								//std::cout << str.substr(p) << std::endl;
								break;
							}
							p = np+1;
						}
					}

				}
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

			// don't use 't' and 's' because they are already used by the
			// testing framework
			case 's':
			case 't':
			default:
				break;
		}
	}

	if (InitializeTests) {
		if (ret = InitializeTests(test_args_list) > 0) {
			// function InitializeTests returns a value > 0 when it wants 
			// to exit immediately with status 0
			return 0;
		}
	}

	if (!xdst) {
		return -1;
	}

	dbg_init(debug_level, NULL);

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);
	
	getTest(argc, argv, &suiteName, &testName);
	libfs_init(principal_id, xdst);
	ret = runTests(suiteName, testName);
	libfs_shutdown();
	return ret;
}
