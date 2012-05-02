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

int Create(int debug_level, const char* xdst);
int Open(int debug_level, const char* xdst);


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


	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	opterr=0;
	while ((ch = getopt(argc, argv, "d:h:b:"))!=-1) {
		switch (ch) {
			case 'd':
				debug_level = atoi(optarg);
				break;
			case 'h':
				xdst = optarg;
				break;
			case 'b':
				ubench = optarg;
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
		return Create(debug_level, xdst);
	}
	if (strcmp(ubench, "open") == 0) {
		return Open(debug_level, xdst);
	}

	return ret;
}
