#ifndef _KVFS_FIXTURE_HXX_AGL189
#define _KVFS_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "kvfs/client/client.h"
#include "kvfs/client/session.h"

using namespace client;

struct KVFSFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	Session*               session;

	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			libfs_shutdown();
		}
	};

	KVFSFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			libfs_init2("10000");
			session = new Session(global_storage_system);
			initialized = true;
			// register a finalize action to be called by the test-framework 
			// when all threads complete
			Finalize* functor = new Finalize();
			TESTFW->RegisterFinalize(functor);
		}
		pthread_mutex_unlock(&mutex);
	}

	~KVFSFixture() 
	{ }
};


#endif // _KVFS_FIXTURE_HXX_AGL189
