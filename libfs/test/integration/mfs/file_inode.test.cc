#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/main/client/omgr.h"
#include "ssa/containers/byte/container.h"
#include "ssa/containers/typeid.h"
#include "test/integration/ssa/obj.fixture.h"
#include "pxfs/mfs/client/file_inode.h"
#include "mfs.fixture.h"


static ssa::common::ObjectId OID[16];

typedef ssa::containers::client::ByteContainer ByteContainer;

static const char* storage_pool_path = "/tmp/stamnos_pool";

SUITE(MFSFileInode)
{
	TEST_FIXTURE(MFSFixture, TestWrite1)
	{
		char                               buf[512];
		::client::Inode*                   inode;
		ssa::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;

		// FIXME
		// ugly hack: to load the storage pool/allocator we mount the pool as a filesystem.
		// instead the ssa layer should allow us to mount just the storage system 
		CHECK(libfs_mount(storage_pool_path, "/home/hvolos", "mfs", 0) == 0);


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_ssa_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
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

	TEST_FIXTURE(MFSFixture, TestRead1)
	{
		::client::Inode*                   inode;
		ssa::common::ObjectProxyReference* rw_ref;
		ByteContainer::Reference*          rw_reft;
		::mfs::client::FileInode*          finode;

		// FIXME
		// ugly hack: to load the storage pool/allocator we mount the pool as a filesystem.
		// instead the ssa layer should allow us to mount just the storage system 
		CHECK(libfs_mount(storage_pool_path, "/home/hvolos", "mfs", 0) == 0);


		EVENT("BeforeMapObjects");
		CHECK(MapObjects<ByteContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_ssa_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
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
