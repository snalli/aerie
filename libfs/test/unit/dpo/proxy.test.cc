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

/*
// Derived proxy object (just for testing inheritance instantiation at compile time)
class DummyRWT : public dpo::client::rw::ObjectProxyTemplate<DummyRWT, Dummy, DummyVersionManager>
{

};

// Derived proxy object reference (just for testing inheritance instantiation at compile time)
class DummyRWReferenceT: public dpo::client::rw::ObjectProxyReferenceTemplate<DummyRWReferenceT, Dummy, DummyVersionManager>
{

};

*/

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
		mgr->Register(TYPE_DUMMY, dummy_mgr);

		Dummy            dummy;        // the public object
		DummyRWReference dummy_rw_ref; // a reference to a private object
		
		CHECK(mgr->GetObject(dummy.oid(), &dummy_rw_ref) == E_SUCCESS);
		//mgr->Make(DummyCoW, &dummy_cow);
		//mgr->MakeDefer(DummyCoW, &dummy_cow);

		//dummy_cow = DummyCoW::Make(dummy);

		//dummy_cow->Open(); // dummy_cow->Lock() or dummy_cow->Lock(parent_oid);
	}


}

