#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "osd/main/client/rwproxy.h"
#include "osd/main/client/omgr.h"
#include "osd/containers/byte/container.h"
#include "test/integration/osd/osd.fixture.h"
#include "pxfs/mfs/client/file_inode.h"
#include "pxfs/common/publisher.h"
#include "mfs.fixture.h"


static osd::common::ObjectId OID[16];

typedef osd::containers::client::ByteContainer ByteContainer;

static const char* storage_pool_path = "/tmp/stamnos_pool";

SUITE(MFSFileInode)
{
	TEST_FIXTURE(MFSFixture, TestWrite1)
	{
		char                               buf[8192];
		::client::Inode*                   inode;
		osd::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;

		// FIXME
		// ugly hack: to load the storage pool/allocator we mount the pool as a filesystem.
		// instead the osd layer should allow us to mount just the storage system 
		CHECK(libfs_mount(storage_pool_path, "/home/hvolos", "mfs", 0) == 0);


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_storage_system->omgr()->GetObject(session, OID[0], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<ByteContainer::Reference*>(rw_ref);
		finode = new ::mfs::client::FileInode(rw_ref);
		
		EVENT("BeforeLock");
		finode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");

		strcpy(buf, "WRITE");
		//finode->Write(session, buf, 0, strlen(buf)+1);

		session->journal()->TransactionBegin();
		session->journal() << Publisher::Message::LogicalOperation::Write(finode->ino());
		finode->Write(session, buf, 4096*11, 5000);
		session->journal()->TransactionCommit();

		finode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

	TEST_FIXTURE(MFSFixture, TestRead1)
	{
		char                               buf[512];
		::client::Inode*                   inode;
		osd::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;

		// FIXME
		// ugly hack: to load the storage pool/allocator we mount the pool as a filesystem.
		// instead the osd layer should allow us to mount just the storage system 
		CHECK(libfs_mount(storage_pool_path, "/home/hvolos", "mfs", 0) == 0);


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_storage_system->omgr()->GetObject(session, OID[0], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<ByteContainer::Reference*>(rw_ref);
		finode = new ::mfs::client::FileInode(rw_ref);
		
		EVENT("BeforeLock");
		finode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");
		
		finode->Read(session, buf, 4096*11, 6);
		CHECK(strcmp(buf, "WRITE") == 0);

		finode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

	TEST_FIXTURE(MFSFixture, TestRead2)
	{
		char                               buf[512];
		::client::Inode*                   inode;
		osd::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;

		// FIXME
		// ugly hack: to load the storage pool/allocator we mount the pool as a filesystem.
		// instead the osd layer should allow us to mount just the storage system 
		CHECK(libfs_mount(storage_pool_path, "/home/hvolos", "mfs", 0) == 0);


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_storage_system->omgr()->GetObject(session, OID[0], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<ByteContainer::Reference*>(rw_ref);
		finode = new ::mfs::client::FileInode(rw_ref);
		
		EVENT("BeforeLock");
		finode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");
		
		finode->Read(session, buf, 0, 6);
		CHECK(strcmp(buf, "WRITE") == 0);

		finode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}



	TEST_FIXTURE(MFSFixture, TestWriteMultiple)
	{
		char                               buf[512];
		::client::Inode*                   inode;
		osd::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode[8];

		// FIXME
		// ugly hack: to load the storage pool/allocator we mount the pool as a filesystem.
		// instead the osd layer should allow us to mount just the storage system 
		CHECK(libfs_mount(storage_pool_path, "/home/hvolos", "mfs", 0) == 0);


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID, 0, 16) == 0);
		EVENT("AfterMapObjects");
		
		for (int i=0; i<8; i++) {
			CHECK(global_storage_system->omgr()->GetObject(session, OID[i], &rw_ref) == E_SUCCESS);
			rw_reft = static_cast<ByteContainer::Reference*>(rw_ref);
			finode[i] = new ::mfs::client::FileInode(rw_ref);
		}

		EVENT("BeforeLock");
		for (int i=7; i>=0; i--) {
			finode[i]->Lock(session, lock_protocol::Mode::XL);
			strcpy(buf, "WRITE");
			// do the logical journaling ourselves because we bypass the file system interface
			session->journal()->TransactionBegin();
			session->journal() << Publisher::Message::LogicalOperation::Write(finode[i]->ino());
			finode[i]->Write(session, buf, 0, strlen(buf)+1);
			session->journal()->TransactionCommit();
			finode[i]->Unlock(session);
		}
		EVENT("AfterUnlock");
		EVENT("End");
	}
}
