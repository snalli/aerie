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


struct ubench_desc {
	const char*   name;
	int         (*function)(int, char* []);
	int           argc;
	char**        argv;
};

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
	printf("usage: %s   %s\n", name                    , "[OPTION] [+UBENCH [UBENCH_OPTION]]");
	//printf("  or:  %s   %s\n", WHITESPACE(strlen(name)), "");

	//printf(
	exit(1);
}


int
main(int argc, char *argv[])
{
	int                generic_argc = 0;
	int                ubench_cnt = 0;
	int                last_ubench_loc = 0;
	pthread_attr_t     attr;
	int                ret = -1;
	int                debug_level = -1;
	uid_t              principal_id;
	char               ch = 0;
	const char*        xdst="10000";
	extern int         opterr;
	extern char*       optarg;
	std::string        unused;
	struct ubench_desc ubench[16];


	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	principal_id = getuid();

	RegisterUbench();
	//printf("\n Sanketh : Return from ubench RegisterUbench() : %d \n", RegisterUbench());
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '+') {
			if (ubench_cnt>0) {
				ubench[ubench_cnt-1].argc = i - last_ubench_loc;
			} else {
				generic_argc = i;
			}
			last_ubench_loc = i;
			ubench[ubench_cnt].argv = &argv[i];
			ubench[ubench_cnt].name = &argv[i][1];
			for (int j=0; j < ubench_table.size(); j++) {
				if (strcmp(ubench_table[j].ubench_name.c_str(), ubench[ubench_cnt].name) == 0) {
					ubench[ubench_cnt].function = ubench_table[j].ubench_function;
					break;
				}
			}
			ubench_cnt++;
		}
	}
	if (ubench_cnt > 0) {
		ubench[ubench_cnt - 1].argc = argc - last_ubench_loc;
	}
	// any arguments that appear before the name of the microbenchmark are 
	// interpreted here. 
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

	if (!ubench_cnt) {
		usage(progname);
		return -1;
	}

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);
	
	printf("INIT\n");
	if ((ret = Init(debug_level, xdst)) < 0) {
		return ret;
	}
	//printf("\n Sanketh : Return from ubench Init() : %d \n", ret);
	//printf("\n Sanketh : ubench_cnt = %d \n", ubench_cnt);
	for (int i=0; i < ubench_cnt; i++) {
		printf("Invoking %s...\n", ubench[i].name);
		if ((ret = ubench[i].function(ubench[i].argc, ubench[i].argv)) < 0) {
			break;
		}
	}


	//printf("\n Sanketh : Return from ubench ShutDown() : %d \n", ShutDown());
	if (ret < 0) {
		fprintf(stderr, "FAILURE\n");
	}
	return ret;
}
