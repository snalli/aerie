#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/client/rwproxy.h"
#include "dpo/client/omgr.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "obj.fixture.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

using namespace client;

const int TYPE_DUMMY = 1;

// Object
class Dummy: public dpo::cc::common::Object {
public:
	Dummy() 
	{ 
		set_type(TYPE_DUMMY);
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

	int vUpdate() {
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

// Client C1:T1 constructs objects, rest just map the region containing 
// the objects
int MapObjects(testfw::Test* test)
{
	void*       ptr;
	void*       base_addr;
	void*       mmap_base_addr=(void*) 0x0000100000000000LLU;
	
	int fd = open("/tmp/stamnos_test_object_file", O_RDWR);
	base_addr = mmap(mmap_base_addr, 1024*1024, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (base_addr != mmap_base_addr) {
		return -1;
	}
	if (strcmp(test->Tag(), "C1:T1") == 0) {
		for (int i=0; i<16; i++) {
			void*  ptr = (void*) (((uint64_t) mmap_base_addr) + i*sizeof(Dummy));
			Dummy* dummy_ptr = new (ptr) Dummy;
			if (dummy_ptr == NULL) {
				return -1;
			}
		}
	}
	for (int i=0; i<16; i++) {
		void* ptr = (void*) (((uint64_t) mmap_base_addr) + i*sizeof(Dummy));
		OID[i] = dpo::common::ObjectId(TYPE_DUMMY, ptr);
	}
	return 0;
}


SUITE(Object)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		DummyRWReference dummy_rw_ref;

		CHECK(MapObjects(SELF) == 0);
		EVENT("AfterMapObjects");
		
		dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>* dummy_mgr = new dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>;
		CHECK(global_omgr->RegisterType(TYPE_DUMMY, dummy_mgr) == E_SUCCESS);
		CHECK(global_omgr->GetObject(OID[0], &dummy_rw_ref) == E_SUCCESS);
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
