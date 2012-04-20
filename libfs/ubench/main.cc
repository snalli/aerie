#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <vector>
#include <getopt.h>
#include <iostream>
#include <list>
#include "ubench/main.h"
#include <sched.h>


const char*                   progname = "ubench";
std::vector<UbenchDescriptor> ubench_table;

extern int RegisterUbench();
extern int Init(int debug_level, const char* xdst);
extern int ShutDown();


/*
static 
int pin_to_core(int core) {
	cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(core, &cpu_mask);

    if(sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) < 0) {
      DBG_LOG(DBG_ERROR, DBG_MODULE(rpc), "Error while setting the CPU affinity: core=%d\n", core);
      return -1;
    }
    return 0;
}
*/


static void 
usage(const char* name)
{
	printf("usage: %s   %s\n", name                    , "[OPTION] +UBENCH [UBENCH_OPTION]");
	//printf("  or:  %s   %s\n", WHITESPACE(strlen(name)), "");

	//printf(
	exit(1);
}


int
main(int argc, char *argv[])
{
	pthread_attr_t    attr;
	int               ret = -1;
	int               debug_level = -1;
	uid_t             principal_id;
	char              ch = 0;
	const char*       xdst="10000";
	char*             ubench_name = NULL;
	extern int        opterr;
	extern char*      optarg;
	std::string       unused;
	int               generic_argc;
	int               (*ubench_function)(int, char* []);


	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	principal_id = getuid();

	// any arguments that appear before the name of the microbenchmark are 
	// interpreted here. 

	generic_argc = argc;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '+') {
			generic_argc = i;
			break;
		}
	}
	if (generic_argc != argc) {
		ubench_name = &argv[generic_argc][1];
	}
	opterr=0;
	while ((ch = getopt(generic_argc, argv, "d:h:t:"))!=-1) {
		switch (ch) {
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'h':
				xdst = optarg;
				break;
			case '?':
				usage(progname);
			default:
				break;
		}
	}

	if (!ubench_name) {
		usage(progname);
	}

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);
	
	RegisterUbench();
	for (int i=0; i < ubench_table.size(); i++) {
		if (strcmp(ubench_table[i].ubench_name.c_str(), ubench_name) == 0) {
			ubench_function = ubench_table[i].ubench_function;
			break;
		}
	}
	if (!ubench_name) {
		return -1;
	}

	if ((ret = Init(debug_level, xdst)) < 0) {
		return ret;
	}

	printf("Invoking %s...\n", ubench_name);
	ret = ubench_function(argc - generic_argc, &argv[generic_argc]);

	ShutDown();

	if (ret < 0) {
		fprintf(stderr, "FAILURE\n");
	}
	return ret;
}
