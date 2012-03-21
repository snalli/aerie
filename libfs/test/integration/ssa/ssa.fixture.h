#ifndef __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H
#define __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <sstream>
#include "tool/testfw/integrationtest.h"
#include "ssa/main/client/registry.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/salloc.h"
#include "test/integration/bcs/ipc.fixture.h"
#include "lock.fixture.h"
#include "pxfs/client/fsomgr.h"
#include "pxfs/mfs/client/mfs.h"

using namespace client;

struct SsaFixture: public LockRegionFixture, IPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	Session*               session;
	
	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			global_storage_system->Close();
			delete global_storage_system;
		}
	};

	SsaFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			global_storage_system = new ssa::client::StorageSystem(global_ipc_layer);
			global_storage_system->Init();
			session = new Session(global_storage_system);
			global_session = session;
			assert(global_storage_system->Mount("/tmp/stamnos_pool", NULL, 0) == 0);
			initialized = true;
			// register a finalize action to be called by the test-framework 
			// when all threads complete
			Finalize* functor = new Finalize();
			TESTFW->RegisterFinalize(functor);

		}
		pthread_mutex_unlock(&mutex);
	}

	~SsaFixture() 
	{ }
};


template<class T>
int 
MapObjects(Session* session, testfw::Test* test, ssa::common::ObjectId* oid_table)
{
	char buf[128];

	if (strcmp(test->Tag(), "C1:T1") == 0) {
		for (int i=0; i<16; i++) {
			T* optr = T::Make(session);
			sprintf(buf, "Object_%d", i);
			global_storage_system->registry()->Add(buf, optr->oid());
		}
	}
	for (int i=0; i<16; i++) {
		ssa::common::ObjectId oid;
		sprintf(buf, "Object_%d", i);
		assert(global_storage_system->registry()->Lookup(buf, &oid) == 0);
		oid_table[i] = oid;
	}
	return 0;
}


#endif // __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H
