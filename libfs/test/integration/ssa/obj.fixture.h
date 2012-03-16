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
#include "ssa/main/client/registry.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/salloc.h"
#include "test/integration/ipc/ipc.fixture.h"
#include "lock.fixture.h"

#include "client/fsomgr.h"
#include "mfs/client/mfs.h"

using namespace client;

struct ObjectFixture: public LockRegionFixture, IPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	Session*               session;
	
	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			global_ssa_layer->Close();
			delete global_ssa_layer;
		}
	};

	ObjectFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			global_ssa_layer = new ssa::client::Dpo(global_ipc_layer);
			global_ssa_layer->Init();
			

			session = new Session(global_ssa_layer);
			global_session = session;

			// HACK: initialize state needed by the file system;
			global_namespace = new NameSpace("GLOBAL");
			assert(global_namespace->Init(global_session) == 0);
			global_fsomgr = new FileSystemObjectManager();
			mfs::client::RegisterBackend(global_fsomgr);

			assert(global_ssa_layer->Open("/tmp/stamnos_pool", 0) == 0);
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
MapObjects(Session* session, testfw::Test* test, ssa::common::ObjectId* oid_table)
{
	char buf[128];

	if (strcmp(test->Tag(), "C1:T1") == 0) {
		for (int i=0; i<16; i++) {
			T* optr = T::Make(session);
			sprintf(buf, "Object_%d", i);
			global_ssa_layer->registry()->Add(buf, optr->oid());
		}
	}
	for (int i=0; i<16; i++) {
		ssa::common::ObjectId oid;
		sprintf(buf, "Object_%d", i);
		assert(global_ssa_layer->registry()->Lookup(buf, &oid) == 0);
		oid_table[i] = oid;
	}
	return 0;
}


#endif // __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H
