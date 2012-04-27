#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <valgrind/callgrind.h>
#include "ubench/time.h"
#include "ubench/osd/osd.h"

using namespace client;

static int
usage() 
{
	return -1;
}


static int 
__ubench_rpc(osd::client::OsdSession* session, int objtype, int numops)
{
	MEASURE_TIME_PREAMBLE
	int                                             ret = E_SUCCESS;
	unsigned long long                              runtime;
	hrtime_t                                        runtime_cycles;
	int                                             r;
	
	MEASURE_TIME_START
	CALLGRIND_TOGGLE_COLLECT

	for (int i = 0; i < numops; i++) {
		global_ipc_layer->call(IpcProtocol::kRpcServerIsAlive, 0, r);
	}

	CALLGRIND_TOGGLE_COLLECT
	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	std::cout << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	return ret;
}


int
ubench_rpc(int argc, char* argv[])
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
	return __ubench_rpc(session, objtype, numops);
}
