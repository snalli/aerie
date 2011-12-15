#ifndef __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H
#define __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "client/libfs.h"
#include "client/config.h"
#include "client/client_i.h"
#include "dpo/client/omgr.h"
#include "rpc.fixture.h"
#include "lock.fixture.h"

using namespace client;

struct ObjectFixture: public LockRegionFixture, RPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	
	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			delete global_omgr;
			delete global_hlckmgr;
			delete global_lckmgr;
		}
	};

	ObjectFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			global_lckmgr = new LockManager(client::rpc_client, client::rpc_server, client::id);
			global_hlckmgr = new HLockManager(global_lckmgr);
			global_omgr = new dpo::client::ObjectManager(global_lckmgr, global_hlckmgr);
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


#endif // __STAMNOS_TEST_DPO_OBJECT_FIXTURE_H
