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
__ubench_hlock_create(osd::client::OsdSession* session, int numops, bool cache)
{
	MEASURE_TIME_PREAMBLE
	int                      ret = E_SUCCESS;
	unsigned long long       runtime;
	hrtime_t                 runtime_cycles;
	osd::cc::client::HLock** hlock = new osd::cc::client::HLock*[numops];

	MEASURE_TIME_START
	CALLGRIND_TOGGLE_COLLECT

	for (int i = 0; i < numops; i++) {
		hlock[i] = global_storage_system->hlckmgr()->FindOrCreateLock(osd::cc::client::LockId(1, i));
	}

	CALLGRIND_TOGGLE_COLLECT
	MEASURE_TIME_STOP

    MEASURE_TIME_DIFF_USEC(runtime)
    MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	std::cout << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	return ret;
}


static int 
__ubench_hlock(osd::client::OsdSession* session, int numops, bool cache)
{
	MEASURE_TIME_PREAMBLE
	int                     ret = E_SUCCESS;
	unsigned long long      runtime;
	hrtime_t                runtime_cycles;
	osd::cc::client::HLock* hlock;

	if ((ret = global_storage_system->hlckmgr()->Acquire(osd::cc::client::LockId(1, 8), lock_protocol::Mode::XR, 0)) != 0) {
		return ret;
	}
	hlock = global_storage_system->hlckmgr()->FindOrCreateLock(osd::cc::client::LockId(1, 8));
	
	if (cache) {
		for (int i = 0; i < numops; i++) {
			lock_protocol::Mode unused;
			if ((ret = global_storage_system->hlckmgr()->Acquire(osd::cc::client::LockId(1, i+16), hlock, lock_protocol::Mode::XL, 0)) != 0) {
				return ret;
			}
			if ((ret = global_storage_system->hlckmgr()->Release(osd::cc::client::LockId(1, i+16))) < 0) {
				return ret;
			}
		}
	}

	MEASURE_TIME_START
	CALLGRIND_TOGGLE_COLLECT

	for (int i = 0; i < numops; i++) {
		lock_protocol::Mode unused;
		if ((ret = global_storage_system->hlckmgr()->Acquire(osd::cc::client::LockId(1, i+16), hlock, lock_protocol::Mode::XL, 0)) != 0) {
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
__ubench_hlock_object(osd::client::OsdSession* session, int numops, bool cache)
{
	MEASURE_TIME_PREAMBLE
	int                      ret = E_SUCCESS;
	unsigned long long       runtime;
	hrtime_t                 runtime_cycles;
	osd::cc::client::HLock*  phlock;
	osd::cc::client::HLock** hlock = new osd::cc::client::HLock*[numops];

	if ((ret = global_storage_system->hlckmgr()->Acquire(osd::cc::client::LockId(1, 8), lock_protocol::Mode::XR, 0)) != 0) {
		return ret;
	}
	phlock = global_storage_system->hlckmgr()->FindOrCreateLock(osd::cc::client::LockId(1, 8));
	for (int i = 0; i < numops; i++) {
		hlock[i] = global_storage_system->hlckmgr()->FindOrCreateLock(osd::cc::client::LockId(1, i+16));
	}

	if (cache) {
		for (int i = 0; i < numops; i++) {
			lock_protocol::Mode unused;
			if ((ret = global_storage_system->hlckmgr()->Acquire(hlock[i], phlock, lock_protocol::Mode::XL, 0)) != 0) {
				return ret;
			}
			if ((ret = global_storage_system->hlckmgr()->Release(hlock[i])) < 0) {
				return ret;
			}
		}
	}

	CALLGRIND_TOGGLE_COLLECT
	MEASURE_TIME_START

	for (int i = 0; i < numops; i++) {
		lock_protocol::Mode unused;
		if ((ret = global_storage_system->hlckmgr()->Acquire(hlock[i], phlock, lock_protocol::Mode::XL, 0)) != 0) {
			return ret;
		}
	}

	MEASURE_TIME_STOP
	CALLGRIND_TOGGLE_COLLECT

    MEASURE_TIME_DIFF_USEC(runtime)
    MEASURE_TIME_DIFF_CYCLES(runtime_cycles)
	
	std::cout << measure_time_summary(numops, runtime, runtime_cycles) << std::endl;
	return ret;
}




int
ubench_hlock(int argc, char* argv[])
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
			case 'o':  // acquire using lock objects instead of LID
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
	if (object) {
		return __ubench_hlock_object(session, numops, cache);
	}
	return __ubench_hlock(session, numops, cache);
}


int
ubench_hlock_create(int argc, char* argv[])
{
	extern int optind;
	extern int opterr;
	char       ch;
	int        numops = 0;
	int        objtype = 2;
	bool       cache = false;
	
	opterr=0;
	optind=0;
	while ((ch = getopt(argc, argv, "cn:"))!=-1) {
		switch (ch) {
			case 'c':  // use cache lock
				cache = true;
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
	return __ubench_hlock_create(session, numops, cache);
}
