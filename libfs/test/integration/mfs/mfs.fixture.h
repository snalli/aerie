#ifndef _MFS_FIXTURE_HXX_AGL189
#define _MFS_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "client/libfs.h"
#include "client/config.h"
#include "client/client_i.h"

using namespace client;

struct MFSFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;

	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			libfs_shutdown();
		}
	};

	MFSFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			libfs_init("10000");
			initialized = true;
			// register a finalize action to be called by the test-framework 
			// when all threads complete
			Finalize* functor = new Finalize();
			TESTFW->RegisterFinalize(functor);
		}
		pthread_mutex_unlock(&mutex);
	}

	~MFSFixture() 
	{ }
};


#endif // _MFS_FIXTURE_HXX_AGL189
