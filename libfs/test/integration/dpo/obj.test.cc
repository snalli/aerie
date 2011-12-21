#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/base/client/rwproxy.h"
#include "dpo/base/client/omgr.h"
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

	Dummy() 
	{ 
		set_type(1);
		nlink_ = 0;
	}
	
	void* operator new(size_t nbytes, Session* session)
    {
		void* ptr;

        if (session->smgr_->AllocExtent(session, nbytes, &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
        return ptr;
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
		nlink_ = subject()->nlink_;
		return E_SUCCESS;
	}

	int vUpdate(::client::Session* session) {
		subject()->nlink_ = nlink_;
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
		DummyRWReference dummy_rw_ref;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<Dummy>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>* dummy_mgr = new dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>;
		CHECK(global_omgr->RegisterType(1, dummy_mgr) == E_SUCCESS);
		CHECK(global_omgr->GetObject(OID[1], &dummy_rw_ref) == E_SUCCESS);
		EVENT("BeforeLock");
		dummy_rw_ref.obj()->Lock(lock_protocol::Mode::XL);
		int nlink = dummy_rw_ref.obj()->interface()->nlink();
		if (strcmp(SELF->Tag(), "C1:T1")==0) {
			CHECK(nlink == 0);
		} else {
			CHECK(nlink == 1);
		}
		dummy_rw_ref.obj()->interface()->set_nlink(nlink+1);
		dummy_rw_ref.obj()->Unlock();
		EVENT("AfterLock");
		EVENT("End");
	}
}
