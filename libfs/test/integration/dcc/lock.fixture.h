#ifndef _LOCK_FIXTURE_HXX_AGL189
#define _LOCK_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "client/libfs.h"
#include "client/config.h"
#include "client/client_i.h"
#include "rpc.fixture.h"

using namespace client;

struct CountRegion {
	unsigned int    ct[256];
	pthread_mutex_t count_mutex;
};

DEFINE_SHARED_MEMORY_REGION_FIXTURE(LockRegionFixture, CountRegion)

inline int LockRegionFixture::InitRegion(void* args)
{
	CountRegion*        region;
	pthread_mutexattr_t psharedm_attr;
	int                 i;

	region = (CountRegion*) OpenAndMap(__pathname, O_CREAT, sizeof(CountRegion));
    pthread_mutexattr_init(&psharedm_attr);
    pthread_mutexattr_setpshared(&psharedm_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&region->count_mutex, &psharedm_attr);
	for (i=0; i<256; i++) {
		region->ct[i] = 0;
	}
}


struct LockFixture: public LockRegionFixture, RPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;

	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			delete global_lckmgr;
		}
	};

	LockFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			global_lckmgr = new LockManager(client::rpc_client, client::rpc_server, client::id, 0);
			initialized = true;
			// register a finalize action to be called by the test-framework 
			// when all threads complete
			Finalize* functor = new Finalize();
			TESTFW->RegisterFinalize(functor);
		}
		pthread_mutex_unlock(&mutex);
	}

	~LockFixture() 
	{ }
};


#endif /* _LOCK_FIXTURE_HXX_AGL189 */
