#ifndef _LOCK_FIXTURE_HXX_AGL189
#define _LOCK_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/ut_barrier.h"
#include "client/libfs.h"

struct CountRegion {
	unsigned int    ct[256];
	pthread_mutex_t count_mutex;
	ut_barrier_t    barrier;
};

DEFINE_SHARED_MEMORY_REGION_FIXTURE(LockFixture,CountRegion)

inline int LockFixture::InitRegion(void* args)
{
	CountRegion*        region;
	pthread_mutexattr_t psharedm_attr;
	int                 i;
	int                 clients_count = reinterpret_cast<long long int>(args);

	region = (CountRegion*) OpenAndMap(__pathname, O_CREAT, sizeof(CountRegion));
    pthread_mutexattr_init(&psharedm_attr);
    pthread_mutexattr_setpshared(&psharedm_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&region->count_mutex, &psharedm_attr);
	ut_barrier_init(&region->barrier, clients_count, 1);
	for (i=0; i<256; i++) {
		region->ct[i] = 0;
	}
}


#endif /* _LOCK_FIXTURE_HXX_AGL189 */
