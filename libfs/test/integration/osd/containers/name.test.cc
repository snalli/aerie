#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "osd/main/client/rwproxy.h"
#include "osd/main/client/omgr.h"
#include "osd/containers/name/container.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "test/integration/osd/osd.fixture.h"

static osd::common::ObjectId OID[16];

typedef osd::containers::client::NameContainer NameContainer;

static const char* storage_pool_path = "/tmp/stamnos_pool";

SUITE(ContainersNameContainer)
{
	TEST_FIXTURE(OsdFixture, Test)
	{
		osd::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_storage_system->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		EVENT("BeforeLock");
		rw_reft->proxy()->Lock(session, lock_protocol::Mode::XL);
		if (strcmp(SELF->Tag(), "C1:T1")==0) {
			CHECK(rw_reft->proxy()->interface()->Insert(session, "test", OID[2]) == E_SUCCESS);
		} else {
			osd::common::ObjectId oid;
			CHECK(rw_reft->proxy()->interface()->Find(session, "test", &oid) == E_SUCCESS);
		}
		rw_reft->proxy()->Unlock(session);
		EVENT("AfterLock");
		EVENT("End");
	}
}
