#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "dpo/dpo.h"
#include "common/errno.h"

//using namespace client;

class Dummy {
public:
	

private:

};

#if 0

class DummyRW:: public dpo::client::Object<DummyRW>
{


};


SUITE(DPO)
{
	TEST(SimpleObject) 
	{
		dpo::client::ObjectManager* mgr = new dpo::client::ObjectManager();

		Dummy    dummy;     // the public object
		DummyCoW dummy_cow; // the CoW private object

		mgr->Make(DummyCoW, &dummy_cow);
		mgr->MakeDefer(DummyCoW, &dummy_cow);

		dummy_cow = DummyCoW::Make(dummy);

		dummy_cow->Open(); // dummy_cow->Lock() or dummy_cow->Lock(parent_oid);
	}


}


#endif
