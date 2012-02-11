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
#include "client/config.h"
#include "chunkstore/registry.h"
#include "dpo/main/client/omgr.h"
#include "test/integration/ipc/ipc.fixture.h"
#include "lock.fixture.h"

using namespace client;

struct ObjectFixture: public LockRegionFixture, IPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	Session*               session;
	
	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			delete global_dpo_layer;
			delete global_registry;
		}
	};

	ObjectFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			global_dpo_layer = new dpo::client::Dpo(global_ipc_layer);
			global_dpo_layer->Init();
			global_registry = new Registry(global_ipc_layer);
			session = new Session(global_dpo_layer);
			initialized = true;
			// register a finalize action to be called by the test-framework 
			// when all threads complete
			Finalize* functor = new Finalize();
			TESTFW->RegisterFinalize(functor);

		}
		pthread_mutex_unlock(&mutex);
	}

	~ObjectFixture() 
	{ }
};


template<class T>
int 
MapObjects(Session* session, testfw::Test* test, dpo::common::ObjectId* oid_table)
{
	char buf[128];

	if (strcmp(test->Tag(), "C1:T1") == 0) {
		for (int i=0; i<16; i++) {
			T* optr = new(session) T;
			sprintf(buf, "Object_%d", i);
			global_registry->Add(buf, optr->oid().u64());
		}
	}
	for (int i=0; i<16; i++) {
		uint64_t u64;
		sprintf(buf, "Object_%d", i);
		assert(global_registry->Lookup(buf, &u64) == 0);
		oid_table[i] = dpo::common::ObjectId(u64);
	}
	return 0;
}


#endif // __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H
