#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/salloc.h"
#include "ssa/main/common/const.h"
#include "ssa/containers/containers.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "ssa.fixture.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace client;

const char* storage_pool_path = "/tmp/stamnos_pool";

typedef ::ssa::client::SsaSession SsaSession;

// Object
class Dummy: public ssa::cc::common::Object {
public:
	enum { 
		kTypeID = 100 // make sure this does not conflict with any base container types 
	};

	Dummy() 
	{ 
		set_type(kTypeID);
		nlink_ = 0;
	}
	
	static Dummy* Make(SsaSession* session)
    {
		void* ptr;

        if (session->salloc()->AllocateExtent(session, sizeof(Dummy), kData, &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
        return new(ptr) Dummy();
    }

	static Dummy* Make(SsaSession* session, void* ptr) {
		return new(ptr) Dummy();
	}

	int nlink_;
};


class DummyVersionManager: public ssa::vm::client::VersionManager<Dummy> {
public:
	int set_nlink(int nlink) {
		nlink_ = nlink;
	}
	
	int nlink() {
		return nlink_;
	}

	int vOpen() {
		nlink_ = object()->nlink_;
		return E_SUCCESS;
	}

	int vUpdate(SsaSession* session) {
		object()->nlink_ = nlink_;
		return E_SUCCESS;
	}
private:
	int nlink_;
};


// Proxy object
typedef ssa::client::rw::ObjectProxy<Dummy, DummyVersionManager> DummyRW;

// Proxy object reference
typedef ssa::client::rw::ObjectProxyReference<Dummy, DummyVersionManager> DummyRWReference;

static ssa::common::ObjectId OID[16];


SUITE(SSA_Object)
{
	TEST_FIXTURE(SsaFixture, Test)
	{
		ssa::common::ObjectProxyReference* ref;
		DummyRWReference*                  dummy_rw_ref;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<Dummy>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		ssa::client::rw::ObjectManager<Dummy, DummyVersionManager>* dummy_mgr = new ssa::client::rw::ObjectManager<Dummy, DummyVersionManager>;
		CHECK(global_storage_system->omgr()->RegisterType(Dummy::kTypeID, dummy_mgr) == E_SUCCESS);
		CHECK(global_storage_system->omgr()->GetObject(session, OID[1], &ref) == E_SUCCESS);
		EVENT("BeforeLock");
		dummy_rw_ref = static_cast<DummyRWReference*>(ref);
		dummy_rw_ref->proxy()->Lock(session, lock_protocol::Mode::XL);
		int nlink = dummy_rw_ref->proxy()->interface()->nlink();
		if (strcmp(SELF->Tag(), "C1:T1")==0) {
			CHECK(nlink == 0);
		} else {
			CHECK(nlink == 1);
		}
		dummy_rw_ref->proxy()->interface()->set_nlink(nlink+1);
		dummy_rw_ref->proxy()->Unlock(session);
		EVENT("AfterLock");
		EVENT("End");
	}
	
	TEST_FIXTURE(SsaFixture, TestAlloc)
	{
		ssa::common::ObjectProxyReference* ref;
		ssa::common::ObjectId              oid;

		CHECK(global_storage_system->salloc()->AllocateContainer(session, ssa::containers::T_NAME_CONTAINER, &oid) == E_SUCCESS);
	}
}
