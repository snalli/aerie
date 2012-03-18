#ifndef _MFS_FIXTURE_HXX_AGL189
#define _MFS_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "pxfs/client/libfs.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/session.h"

using namespace client;

struct MFSFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;
	Session*               session;

	struct Finalize: testfw::AbstractFunctor {
		void operator()() {
			libfs_shutdown();
		}
	};

	MFSFixture() 
	{
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			libfs_init2("10000");
			
			session = new Session(global_ssa_layer);
			global_session = session;
			
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
