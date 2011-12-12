#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "dpo/dpo.h"
#include "dpo/client/rwproxy.h"
#include "common/errno.h"

using namespace client;

const int TYPE_DUMMY = 1;

// Immutable object
class Dummy: public dpo::cc::common::Object {
public:
	Dummy() 
	{ 
		set_type(TYPE_DUMMY);
	}

	int nlink_;
};

class DummyVersionManager {
public:

};

// Proxy object
typedef dpo::client::rw::ObjectProxy<Dummy, DummyVersionManager> DummyRW;

// Proxy object reference
typedef dpo::client::rw::ObjectProxyReference<Dummy, DummyVersionManager> DummyRWReference;


SUITE(DPO)
{
	TEST(SimpleObject) 
	{
		dpo::client::ObjectManager* mgr = new dpo::client::ObjectManager();
		dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>* dummy_mgr = new dpo::client::rw::ObjectManager<Dummy, DummyVersionManager>;
		CHECK(mgr->Register(TYPE_DUMMY, dummy_mgr) == E_SUCCESS);

		Dummy            dummy;        // the public object
		DummyRWReference dummy_rw_ref; // a reference to a private object
		
		CHECK(mgr->GetObject(dummy.oid(), &dummy_rw_ref) == E_SUCCESS);
		CHECK(dummy_rw_ref.obj()->subject() == &dummy);
		CHECK(mgr->PutObject(dummy_rw_ref) == E_SUCCESS);
	}
}
