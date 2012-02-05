#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/base/client/rwproxy.h"
#include "dpo/base/client/omgr.h"
#include "dpo/containers/byte/container.h"
#include "dpo/containers/typeid.h"
#include "test/integration/dpo/obj.fixture.h"

#include "mfs/client/file_inode.h"


static dpo::common::ObjectId OID[16];

typedef dpo::containers::client::ByteContainer ByteContainer;

SUITE(MFSFileInode)
{
	TEST_FIXTURE(ObjectFixture, TestWrite1)
	{
		char                               buf[512];
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_omgr->GetObject(OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<ByteContainer::Reference*>(rw_ref);
		finode = new ::mfs::client::FileInode(rw_ref);
		
		EVENT("BeforeLock");
		finode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");

		strcpy(buf, "WRITE");
		finode->Write(session, buf, 0, strlen(buf)+1);

		finode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

	TEST_FIXTURE(ObjectFixture, TestRead1)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_omgr->GetObject(OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<ByteContainer::Reference*>(rw_ref);
		finode = new ::mfs::client::FileInode(rw_ref);
		
		EVENT("BeforeLock");
		finode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");
		


		finode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

}
