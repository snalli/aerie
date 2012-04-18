#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <valgrind/callgrind.h>
#include "ubench/osd/main.h"
#include "time.h"

static int
usage() 
{
	return -1;
}


static int 
__ubench_open(osd::client::OsdSession* session, int objtype, int numops)
{
	MEASURE_TIME_PREAMBLE
	int                                              ret = E_SUCCESS;
	osd::containers::client::ByteContainer::Object** obj = (osd::containers::client::ByteContainer::Object**) malloc(sizeof(osd::containers::client::ByteContainer::Object*) * numops);
	osd::common::ObjectProxyReference*               ref;
	unsigned long long                               runtime;
	hrtime_t                                         runtime_cycles;

	for (int i = 0; i < numops; i++) {
		if ((obj[i] = osd::containers::client::ByteContainer::Object::Make(session)) == NULL) {
			return -E_NOMEM;
		}
	}
	
	MEASURE_TIME_START
	CALLGRIND_TOGGLE_COLLECT

	for (int i = 0; i < numops; i++) {
		if ((ret = session->omgr_->GetObject(session, obj[i]->oid(), &ref)) < 0) {
			return ret;
		}
	}

	CALLGRIND_TOGGLE_COLLECT
	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	std::cout << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	return ret;
}


int
ubench_open(int argc, char* argv[])
{
	extern int optind;
	extern int opterr;
	char       ch;
	int        numops = 0;
	int        objtype = 2;
	
	opterr=0;
	optind=0;
	while ((ch = getopt(argc, argv, "t:n:"))!=-1) {
		switch (ch) {
			case 't':
				objtype = atoi(optarg);
				break;
			case 'n':
				numops = atoi(optarg);
				printf("%d\n" , numops);
				break;
			case '?':
				usage();
			default:
				break;
		}
	}
	osd::client::OsdSession* session = new osd::client::OsdSession(global_storage_system);
	__ubench_open(session, objtype, numops);
	return __ubench_open(session, objtype, numops);
}
