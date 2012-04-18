#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <getopt.h>
#include <iostream>
#include <list>
#include "bcs/bcs.h"
#include "ubench/osd/main.h"

extern int ubench_create(int argc, char* argv[]);
extern int ubench_open(int argc, char* argv[]);
extern int ubench_lock(int argc, char* argv[]);
extern int ubench_hlock(int argc, char* argv[]);
extern int ubench_hlock_create(int argc, char* argv[]);

const char*                 poolpath = "/tmp/stamnos_pool";
const char*                 progname = "ubench";
osd::client::StorageSystem* global_storage_system;
Ipc*                        global_ipc_layer;


UbenchDescriptor ubench_table[] = {
	{ "create", ubench_create},
	{ "open", ubench_open},
	{ "lock", ubench_lock},
	{ "hlock", ubench_hlock},
	{ "hlock_create", ubench_hlock_create},
	{ NULL, NULL }
};


int 
Init(const char* xdst)
{
	global_ipc_layer = new client::Ipc(xdst);
	assert((global_ipc_layer->Init()) == 0);
	global_storage_system = new osd::client::StorageSystem(global_ipc_layer);
	assert((global_storage_system->Init()) == 0);
	assert(global_storage_system->Mount(poolpath, NULL, 0) == 0);

	return 0;
}


int
ShutDown()
{

	return 0;
}


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
	char*             xdst=NULL;
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
	while ((ch = getopt(generic_argc, argv, "d:h:"))!=-1) {
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

	if (!xdst || !ubench_name) {
		usage(progname);
	}

	// set stack size to 32K, so we don't run out of memory
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 32*1024);

	for (int i=0; ubench_table[i].ubench_name != NULL; i++) {
		if (strcmp(ubench_table[i].ubench_name, ubench_name) == 0) {
			ubench_function = ubench_table[i].ubench_function;
		}
	}
	if (!ubench_name) {
		return -1;
	}

	Config::Init();
	Debug::Init(debug_level, NULL);

	if ((ret = Init(xdst)) < 0) {
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
