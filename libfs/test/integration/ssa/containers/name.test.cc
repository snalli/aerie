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
#include "ssa/containers/typeid.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "test/integration/ssa/ssa.fixture.h"

static ssa::common::ObjectId OID[16];

typedef ssa::containers::client::NameContainer NameContainer;

static const char* storage_pool_path = "/tmp/stamnos_pool";

SUITE(ContainersNameContainer)
{
	TEST_FIXTURE(SsaFixture, Test)
	{
		ssa::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_storage_system->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		EVENT("BeforeLock");
		rw_reft->proxy()->Lock(session, lock_protocol::Mode::XL);
		if (strcmp(SELF->Tag(), "C1:T1")==0) {
			CHECK(rw_reft->proxy()->interface()->Insert(session, "test", OID[2]) == E_SUCCESS);
		} else {
			ssa::common::ObjectId oid;
			CHECK(rw_reft->proxy()->interface()->Find(session, "test", &oid) == E_SUCCESS);
		}
		rw_reft->proxy()->Unlock(session);
		EVENT("AfterLock");
		EVENT("End");
	}
}
