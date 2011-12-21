#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "mfs/dir_pnode.h"
#include "common/errno.h"
#include "test/unit/fixture/session.fixture.h"

using namespace mfs;

/*
SUITE(MFSDirPnode)
{
	TEST_FIXTURE(SessionFixture, TestLinkLookup)
	{
		uint64_t           ino;
		DirPnode<Session>* dirpnode = new(session) DirPnode<Session>;
		
		CHECK(dirpnode->Link(session, ".", 1) == 0);
		CHECK(dirpnode->Link(session, "..", 0) == 0);
		CHECK(dirpnode->Link(session, "foo", 3) == 0);
		CHECK(dirpnode->Link(session, "bar", 4) == 0);
		
		CHECK(dirpnode->Lookup(session, ".", &ino) == 0);
		CHECK(ino == 1);
		CHECK(dirpnode->Lookup(session, "..", &ino) == 0);
		CHECK(ino == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino) == 0);
		CHECK(ino == 3);
		CHECK(dirpnode->Lookup(session, "bar", &ino) == 0);
		CHECK(ino == 4);
	}


	TEST_FIXTURE(SessionFixture, TestUnlinkDot)
	{
		uint64_t           ino;
		DirPnode<Session>* dirpnode = new(session) DirPnode<Session>;
		
		CHECK(dirpnode->Link(session, ".", 2) == 0);
		CHECK(dirpnode->Link(session, "..", 1) == 0);
		
		CHECK(dirpnode->Lookup(session, ".", &ino) == 0);
		CHECK(ino == 2);
		CHECK(dirpnode->Lookup(session, "..", &ino) == 0);
		CHECK(ino == 1);
		
		CHECK(dirpnode->Unlink(session, ".") == 0);
		CHECK(dirpnode->Lookup(session, ".", &ino) == -E_EXIST);
	
		CHECK(dirpnode->Unlink(session, "..") == 0);
		CHECK(dirpnode->Lookup(session, "..", &ino) == -E_EXIST);
	}

	TEST_FIXTURE(SessionFixture, TestUnlink)
	{
		uint64_t           ino;
		DirPnode<Session>* dirpnode = new(session) DirPnode<Session>;
		
		CHECK(dirpnode->Link(session, ".", 1) == 0);
		CHECK(dirpnode->Link(session, "..", 0) == 0);
		CHECK(dirpnode->Link(session, "foo", 3) == 0);
		CHECK(dirpnode->Link(session, "bar", 4) == 0);
		CHECK(dirpnode->Link(session, "doc", 12) == 0);
		
		CHECK(dirpnode->Lookup(session, ".", &ino) == 0);
		CHECK(ino == 1);
		CHECK(dirpnode->Lookup(session, "..", &ino) == 0);
		CHECK(ino == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino) == 0);
		CHECK(ino == 3);
		CHECK(dirpnode->Lookup(session, "bar", &ino) == 0);
		CHECK(ino == 4);
		CHECK(dirpnode->Lookup(session, "doc", &ino) == 0);
		CHECK(ino == 12);
	
		CHECK(dirpnode->Unlink(session, "foo") == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino) != 0);
		
		CHECK(dirpnode->Unlink(session, "bar") == 0);
		CHECK(dirpnode->Lookup(session, "bar", &ino) != 0);
	
		CHECK(dirpnode->Link(session, "foo", 13) == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino) == 0);
		CHECK(ino == 13);
		CHECK(dirpnode->Link(session, "bar", 14) == 0);
		CHECK(dirpnode->Lookup(session, "bar", &ino) == 0);
		CHECK(ino == 14);
		
		CHECK(dirpnode->Unlink(session, "doc") == 0);
		CHECK(dirpnode->Lookup(session, "doc", &ino) != 0);
	
		CHECK(dirpnode->Unlink(session, "foo") == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino) != 0);
		CHECK(dirpnode->Unlink(session, "bar") == 0);
		CHECK(dirpnode->Lookup(session, "bar", &ino) != 0);
	}


	TEST_FIXTURE(SessionFixture, TestLinkOverwrite)
	{
		uint64_t           ino;
		DirPnode<Session>* dirpnode = new(session) DirPnode<Session>;
		
		CHECK(dirpnode->Link(session, ".", 1) == 0);
		CHECK(dirpnode->Link(session, "..", 0) == 0);
		CHECK(dirpnode->Link(session, "foo", 3) == 0);
		CHECK(dirpnode->Link(session, "bar", 4) == 0);
		
		CHECK(dirpnode->Lookup(session, ".", &ino) == 0);
		CHECK(ino == 1);
		CHECK(dirpnode->Lookup(session, "..", &ino) == 0);
		CHECK(ino == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino) == 0);
		CHECK(ino == 3);
		CHECK(dirpnode->Lookup(session, "bar", &ino) == 0);
		CHECK(ino == 4);
	
		CHECK(dirpnode->Link(session, "foo", 13) != 0);
	}


}

*/
