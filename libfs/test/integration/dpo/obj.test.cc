#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/main/client/rwproxy.h"
#include "dpo/main/client/omgr.h"
#include "dpo/main/client/salloc.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "obj.fixture.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace client;


// Object
class Dummy: public dpo::cc::common::Object {
public:
	enum { 
		kTypeID = 100 // make sure this does not conflict with any base container types 
	};

	Dummy() 
	{ 
		set_type(kTypeID);
		nlink_ = 0;
	}
	
	static Dummy* Make(Session* session)
    {
		void* ptr;

        if (session->salloc_->AllocExtent(session, sizeof(Dummy), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
        return new(ptr) Dummy();
    }

	static Dummy* Make(Session* session, void* ptr) {
		return new(ptr) Dummy();
	}

	int nlink_;
};


class DummyVersionManager: public dpo::vm::client::VersionManager<Dummy> {
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

	int vUpdate(::client::Session* session) {
		object()->nlink_ = nlink_;
		return E_SUCCESS;
	}
private:
	int nlink_;
};


// Proxy object
typedef dpo::client::rw::ObjectProxy<Dummy, DummyVersionManager> DummyRW;

// Proxy object reference
typedef dpo::client::rw::ObjectProxyReference<Dummy, DummyVersionManager> DummyRWReference;

static dpo::common::ObjectId OID[16];

SUITE(Object)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		dpo::common::ObjectProxyReference* ref;
		DummyRWReference*                  dummy_rw_ref;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<Dummy>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>* dummy_mgr = new dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>;
		CHECK(global_dpo_layer->omgr()->RegisterType(Dummy::kTypeID, dummy_mgr) == E_SUCCESS);
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &ref) == E_SUCCESS);
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
}
