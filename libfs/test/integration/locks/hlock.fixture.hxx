#ifndef _HLOCK_FIXTURE_HXX_AGL189
#define _HLOCK_FIXTURE_HXX_AGL189

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/ut_barrier.h"
#include "client/libfs.h"
#include "client/config.h"
#include "client/client_i.h"
#include "rpc.fixture.hxx"
#include "lock.fixture.hxx"

using namespace client;

struct HLockFixture: public LockRegionFixture, RPCFixture {
	HLockFixture() 
	{
		global_lckmgr = new LockManager(client::rpc_client, client::rpc_server, client::id, 0);
		global_hlckmgr = new HLockManager(global_lckmgr);
	}

	~HLockFixture() 
	{
		delete global_lckmgr;
		delete global_hlckmgr;
	}
};


#endif /* _HLOCK_FIXTURE_HXX_AGL189 */
