#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "dpo/dpo.h"
#include "dpo/client/rwproxy.h"
#include "common/errno.h"

using namespace client;

// Immutable object
class Dummy {
public:
	
	int nlink_;
};

class DummyVersionManager {
public:

	

};

// Proxy object
//template dpo::client::rw::ObjectProxy<Dummy, DummyVersionManager> DummyRW;
class DummyRW : public dpo::client::rw::ObjectProxy<DummyRW, Dummy, DummyVersionManager>
{

};


SUITE(DPO)
{
	TEST(SimpleObject) 
	{
		//dpo::client::ObjectManager* mgr = new dpo::client::ObjectManager();

		//Dummy    dummy;    // the public object
		//DummyRW  dummy_rw; // the RW private object

		//mgr->Make(DummyCoW, &dummy_cow);
		//mgr->MakeDefer(DummyCoW, &dummy_cow);

		//dummy_cow = DummyCoW::Make(dummy);

		//dummy_cow->Open(); // dummy_cow->Lock() or dummy_cow->Lock(parent_oid);
	}


}

