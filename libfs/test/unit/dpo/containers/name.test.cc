#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "test/unit/fixture/session.fixture.h"
#include "dpo/containers/name/container.h"


using namespace dpo::containers::common;
	
SUITE(ContainersNameContainer)
{
	TEST_FIXTURE(SessionFixture, TestInsertLookup)
	{
		dpo::common::ObjectId           oid;
		NameContainer::Object<Session>* name_container = new(session) NameContainer::Object<Session>;
		
		CHECK(name_container->Insert(session, ".", dpo::common::ObjectId(1)) == 0);
		CHECK(name_container->Insert(session, "..", dpo::common::ObjectId(2)) == 0);
		CHECK(name_container->Insert(session, "foo", dpo::common::ObjectId(3)) == 0);
		CHECK(name_container->Insert(session, "bar", dpo::common::ObjectId(4)) == 0);
		
		CHECK(name_container->Find(session, ".", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(1));
		CHECK(name_container->Find(session, "..", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(2));
		CHECK(name_container->Find(session, "foo", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(3));
		CHECK(name_container->Find(session, "bar", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(4));
	}

	TEST_FIXTURE(SessionFixture, TestEraseDot)
	{
		dpo::common::ObjectId           oid;
		NameContainer::Object<Session>* name_container = new(session) NameContainer::Object<Session>;
		
		CHECK(name_container->Insert(session, ".", dpo::common::ObjectId(2)) == 0);
		CHECK(name_container->Insert(session, "..", dpo::common::ObjectId(1)) == 0);
		
		CHECK(name_container->Find(session, ".", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(2));
		CHECK(name_container->Find(session, "..", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(1));
		
		CHECK(name_container->Erase(session, ".") == 0);
		CHECK(name_container->Find(session, ".", &oid) == -E_EXIST);
	
		CHECK(name_container->Erase(session, "..") == 0);
		CHECK(name_container->Find(session, "..", &oid) == -E_EXIST);
	}

	TEST_FIXTURE(SessionFixture, TestErase)
	{
		dpo::common::ObjectId           oid;
		NameContainer::Object<Session>* name_container = new(session) NameContainer::Object<Session>;
		
		CHECK(name_container->Insert(session, ".", dpo::common::ObjectId(1)) == 0);
		CHECK(name_container->Insert(session, "..", dpo::common::ObjectId(2)) == 0);
		CHECK(name_container->Insert(session, "foo", dpo::common::ObjectId(3)) == 0);
		CHECK(name_container->Insert(session, "bar", dpo::common::ObjectId(4)) == 0);
		CHECK(name_container->Insert(session, "doc", dpo::common::ObjectId(12)) == 0);
		
		CHECK(name_container->Find(session, ".", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(1));
		CHECK(name_container->Find(session, "..", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(2));
		CHECK(name_container->Find(session, "foo", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(3));
		CHECK(name_container->Find(session, "bar", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(4));
		CHECK(name_container->Find(session, "doc", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(12));
	
		CHECK(name_container->Erase(session, "foo") == 0);
		CHECK(name_container->Find(session, "foo", &oid) != 0);
		
		CHECK(name_container->Erase(session, "bar") == 0);
		CHECK(name_container->Find(session, "bar", &oid) != 0);
	
		CHECK(name_container->Insert(session, "foo", 13) == 0);
		CHECK(name_container->Find(session, "foo", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(13));
		CHECK(name_container->Insert(session, "bar", 14) == 0);
		CHECK(name_container->Find(session, "bar", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(14));
		
		CHECK(name_container->Erase(session, "doc") == 0);
		CHECK(name_container->Find(session, "doc", &oid) != 0);
	
		CHECK(name_container->Erase(session, "foo") == 0);
		CHECK(name_container->Find(session, "foo", &oid) != 0);
		CHECK(name_container->Erase(session, "bar") == 0);
		CHECK(name_container->Find(session, "bar", &oid) != 0);
	}


	TEST_FIXTURE(SessionFixture, TestInsertOverwrite)
	{
		dpo::common::ObjectId           oid;
		NameContainer::Object<Session>* name_container = new(session) NameContainer::Object<Session>;
		
		CHECK(name_container->Insert(session, ".", dpo::common::ObjectId(1)) == 0);
		CHECK(name_container->Insert(session, "..", dpo::common::ObjectId(2)) == 0);
		CHECK(name_container->Insert(session, "foo", dpo::common::ObjectId(3)) == 0);
		CHECK(name_container->Insert(session, "bar", dpo::common::ObjectId(4)) == 0);
		
		CHECK(name_container->Find(session, ".", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(1));
		CHECK(name_container->Find(session, "..", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(2));
		CHECK(name_container->Find(session, "foo", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(3));
		CHECK(name_container->Find(session, "bar", &oid) == 0);
		CHECK(oid == dpo::common::ObjectId(4));
	
		CHECK(name_container->Insert(session, "foo", dpo::common::ObjectId(13)) != 0);
	}

}
