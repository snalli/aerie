#ifndef _CFS_FIXTURE_HXX_AGL189
#define _CFS_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "cfs/client/client_i.h"
#include "cfs/client/libfs.h"
#include "cfs/client/session.h"

using namespace client;

struct CFSFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	Session*               session;

	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			cfs_shutdown();
		}
	};

	CFSFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			cfs_init2("10000");
			session = new Session(global_storage_system);
			initialized = true;
			// register a finalize action to be called by the test-framework 
			// when all threads complete
			Finalize* functor = new Finalize();
			TESTFW->RegisterFinalize(functor);
		}
		pthread_mutex_unlock(&mutex);
	}

	~CFSFixture() 
	{ }
};


#endif // _CFS_FIXTURE_HXX_AGL189
