#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/base/client/rwproxy.h"
#include "dpo/base/client/omgr.h"
#include "dpo/containers/name/container.h"
#include "dpo/containers/typeid.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "test/integration/dpo/obj.fixture.h"

static dpo::common::ObjectId OID[16];

typedef dpo::containers::client::NameContainer NameContainer;

SUITE(ContainersNameContainer)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		dpo::client::rw::ObjectManager<NameContainer::Object, NameContainer::VersionManager>* mgr = new dpo::client::rw::ObjectManager<NameContainer::Object, NameContainer::VersionManager>;
		CHECK(global_omgr->RegisterType(dpo::containers::T_NAME_CONTAINER, mgr) == E_SUCCESS);
		CHECK(global_omgr->GetObject(OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		EVENT("BeforeLock");
		rw_reft->obj()->Lock(lock_protocol::Mode::XL);
		if (strcmp(SELF->Tag(), "C1:T1")==0) {
			CHECK(rw_reft->obj()->interface()->Insert(session, "test", OID[2]) == E_SUCCESS);
		} else {
			dpo::common::ObjectId oid;
			CHECK(rw_reft->obj()->interface()->Find(session, "test", &oid) == E_SUCCESS);
		}
		rw_reft->obj()->Unlock();
		EVENT("AfterLock");
		EVENT("End");
	}
}
