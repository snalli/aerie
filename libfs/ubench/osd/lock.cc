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
__ubench_lock(osd::client::OsdSession* session, int numops, bool cache)
{
	MEASURE_TIME_PREAMBLE
	int                  ret = E_SUCCESS;
	unsigned long long   runtime;
	hrtime_t             runtime_cycles;

	if (cache) {
		for (int i = 0; i < numops; i++) {
			lock_protocol::Mode unused;
			if ((ret = global_storage_system->lckmgr()->Acquire(osd::cc::client::LockId(i+16), lock_protocol::Mode::XL, 0, unused)) < 0) {
				return ret;
			}
			if ((ret = global_storage_system->lckmgr()->Release(osd::cc::client::LockId(i+16))) < 0) {
				return ret;
			}
		}
	}

	MEASURE_TIME_START
	CALLGRIND_TOGGLE_COLLECT

	for (int i = 0; i < numops; i++) {
		lock_protocol::Mode unused;
		if ((ret = global_storage_system->lckmgr()->Acquire(osd::cc::client::LockId(i+16), lock_protocol::Mode::XL, 0, unused)) < 0) {
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


static int 
__ubench_lock_object(osd::client::OsdSession* session, int numops, bool cache)
{
	MEASURE_TIME_PREAMBLE
	int                     ret = E_SUCCESS;
	osd::cc::client::Lock** lock_tbl = (osd::cc::client::Lock**) malloc(sizeof(osd::cc::client::Lock*) * numops);
	unsigned long long      runtime;
	hrtime_t                runtime_cycles;
	
	for (int i = 0; i < numops; i++) {
		if ((lock_tbl[i] = global_storage_system->lckmgr()->FindOrCreateLock(osd::cc::client::LockId(i+16))) == NULL) {
			return -1;
		}
	}
	
	if (cache) {
		for (int i = 0; i < numops; i++) {
			lock_protocol::Mode unused;
			if ((ret = global_storage_system->lckmgr()->Acquire(lock_tbl[i+16], lock_protocol::Mode::XL, 0, unused)) < 0) {
				return ret;
			}
			if ((ret = global_storage_system->lckmgr()->Release(osd::cc::client::LockId(i+16))) < 0) {
				return ret;
			}
		}
	}

	MEASURE_TIME_START
	CALLGRIND_TOGGLE_COLLECT

	for (int i = 0; i < numops; i++) {
		lock_protocol::Mode unused;
		if ((ret = global_storage_system->lckmgr()->Acquire(lock_tbl[i+16], lock_protocol::Mode::XL, 0, unused)) < 0) {
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
ubench_lock(int argc, char* argv[])
{
	extern int optind;
	extern int opterr;
	char       ch;
	int        numops = 0;
	int        objtype = 2;
	bool       cache = false;
	bool       object = false;
	
	opterr=0;
	optind=0;
	while ((ch = getopt(argc, argv, "ocn:"))!=-1) {
		switch (ch) {
			case 'c':  // use cache lock
				cache = true;
				break;
			case 'o': // create lock objects before we acquire them
				object = true;
				break;
			case 'n':
				numops = atoi(optarg);
				break;
			case '?':
				usage();
			default:
				break;
		}
	}
	osd::client::OsdSession* session = new osd::client::OsdSession(global_storage_system);
	return __ubench_lock(session, numops, cache);
}
