#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/main/client/omgr.h"
#include "ssa/containers/name/container.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "test/integration/ssa/ssa.fixture.h"

static ssa::common::ObjectId OID[16];

typedef ssa::containers::client::NameContainer NameContainer;

static const char* storage_pool_path = "/tmp/stamnos_pool";

SUITE(SSA_STM)
{
	// uses locks to update the container
	TEST_FIXTURE(SsaFixture, TestPessimisticUpdate)
	{
		ssa::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		ssa::client::rw::ObjectManager<NameContainer::Object, NameContainer::VersionManager>* mgr = new ssa::client::rw::ObjectManager<NameContainer::Object, NameContainer::VersionManager>;
		CHECK(global_storage_system->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		EVENT("BeforeLock");
		rw_reft->proxy()->Lock(session, lock_protocol::Mode::XL);
		CHECK(rw_reft->proxy()->interface()->Insert(session, "test", OID[2]) == E_SUCCESS);
		EVENT("BeforeUnlock");
		rw_reft->proxy()->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

	// uses transactions to optimistically read the container
	TEST_FIXTURE(SsaFixture, TestOptimisticRead)
	{
		ssa::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		ssa::client::rw::ObjectManager<NameContainer::Object, NameContainer::VersionManager>* mgr = new ssa::client::rw::ObjectManager<NameContainer::Object, NameContainer::VersionManager>;
		CHECK(global_storage_system->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		STM_BEGIN()
			ssa::common::ObjectId oid;
			CHECK(rw_reft->proxy()->xOpenRO() != NULL);
			CHECK(rw_reft->proxy()->xinterface()->Find(session, "test", &oid) != E_SUCCESS);
			EVENT("AfterFind");
			CHECK(rw_reft->proxy()->Lock(session, lock_protocol::Mode::XL) == E_SUCCESS); // hack to force the other client to publish the changes
			EVENT("AfterLock");
			CHECK(__tx->Validate() < 0); 
			goto done; // bypass transaction commit as we know it will fail and rollback. 
		STM_END() 
	done:
		EVENT("End");
	}
}
