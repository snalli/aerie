#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/base/client/rwproxy.h"
#include "dpo/base/client/omgr.h"
#include "dpo/containers/name/container.h"
#include "dpo/containers/typeid.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "test/integration/dpo/obj.fixture.h"

#include "mfs/client/dir_inode.h"


static dpo::common::ObjectId OID[16];

typedef dpo::containers::client::NameContainer NameContainer;


SUITE(MFSDirInode)
{
	void InitNames(client::Session* session, client::Inode* dinode) 
	{
		::client::Inode*                     inode;
		::dpo::common::ObjectProxyReference* rw_ref;
		::NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*             cinode;

		/* foo */
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[2], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "foo", cinode, false) == 0);
		/* bar */
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[3], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "bar", cinode, false) == 0);
		/* doc */
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[4], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "doc", cinode, false) == 0);
	}


	TEST_FIXTURE(ObjectFixture, TestLink)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;
		::mfs::client::DirInode*           cinode;


		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
		
		dinode->Lock(session, lock_protocol::Mode::XL);
		InitNames(session, dinode);
		
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);

		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[5], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "media", cinode, false) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "media", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
	}


	TEST_FIXTURE(ObjectFixture, TestUnlink)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;
		::mfs::client::DirInode*           cinode;


		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
		
		dinode->Lock(session, lock_protocol::Mode::XL);
		InitNames(session, dinode);
		
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);

		CHECK(dinode->Unlink(session, "foo") == E_SUCCESS);
		CHECK(dinode->Lookup(session, "foo", 0, &inode) != E_SUCCESS);


		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[5], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "foo", cinode, false) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
	}


	TEST_FIXTURE(ObjectFixture, TestLink1_publisher)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;
		::mfs::client::DirInode*           child1;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
	
		EVENT("BeforeLock");
		dinode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");
		InitNames(session, dinode);

		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[5], &rw_ref) == E_SUCCESS);
		child1 = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "media", child1, false) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "media", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}


	TEST_FIXTURE(ObjectFixture, TestLink1_consumer)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
	
		EVENT("BeforeLock");
		dinode->Lock(session, lock_protocol::Mode::XL);

		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "media", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}


	TEST_FIXTURE(ObjectFixture, TestLink2_publisher)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;
		::mfs::client::DirInode*           child1;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
	
		EVENT("BeforeLock");
		dinode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");
		InitNames(session, dinode);

		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);
		
		CHECK(dinode->Unlink(session, "foo") == E_SUCCESS);

		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[5], &rw_ref) == E_SUCCESS);
		child1 = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "foo", child1, false) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}


	TEST_FIXTURE(ObjectFixture, TestLink2_consumer)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
	
		EVENT("BeforeLock");
		dinode->Lock(session, lock_protocol::Mode::XL);

		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}


	TEST_FIXTURE(ObjectFixture, TestUnlink1_publisher)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;
		::mfs::client::DirInode*           child1;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
	
		EVENT("BeforeLock");
		dinode->Lock(session, lock_protocol::Mode::XL);
		EVENT("AfterLock");
		InitNames(session, dinode);

		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);
		
		CHECK(dinode->Unlink(session, "foo") == E_SUCCESS);
		
		dinode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

	TEST_FIXTURE(ObjectFixture, TestUnlink1_consumer)
	{
		::client::Inode*                   inode;
		dpo::common::ObjectProxyReference* rw_ref;
		NameContainer::Reference*          rw_reft;
		::mfs::client::DirInode*           dinode;

		EVENT("BeforeMapObjects");
		CHECK(MapObjects<NameContainer::Object>(session, SELF, OID) == 0);
		EVENT("AfterMapObjects");
		
		CHECK(global_dpo_layer->omgr()->GetObject(session, OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
	
		EVENT("BeforeLock");
		dinode->Lock(session, lock_protocol::Mode::XL);

		CHECK(dinode->Lookup(session, "foo", 0, &inode) != E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
		EVENT("AfterUnlock");
		EVENT("End");
	}

}
