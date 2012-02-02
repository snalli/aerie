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
		CHECK(global_omgr->GetObject(OID[2], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "foo", cinode, false) == 0);
		/* bar */
		CHECK(global_omgr->GetObject(OID[3], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "bar", cinode, false) == 0);
		/* doc */
		CHECK(global_omgr->GetObject(OID[4], &rw_ref) == E_SUCCESS);
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
		
		CHECK(global_omgr->GetObject(OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
		
		dinode->Lock(session, lock_protocol::Mode::XL);
		InitNames(session, dinode);
		
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);

		CHECK(global_omgr->GetObject(OID[5], &rw_ref) == E_SUCCESS);
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
		
		CHECK(global_omgr->GetObject(OID[1], &rw_ref) == E_SUCCESS);
		rw_reft = static_cast<NameContainer::Reference*>(rw_ref);
		dinode = new ::mfs::client::DirInode(rw_ref);
		
		dinode->Lock(session, lock_protocol::Mode::XL);
		InitNames(session, dinode);
		
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "bar", 0, &inode) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "doc", 0, &inode) == E_SUCCESS);

		CHECK(dinode->Unlink(session, "foo") == E_SUCCESS);
		CHECK(dinode->Lookup(session, "foo", 0, &inode) != E_SUCCESS);


		CHECK(global_omgr->GetObject(OID[5], &rw_ref) == E_SUCCESS);
		cinode = new ::mfs::client::DirInode(rw_ref);
		CHECK(dinode->Link(session, "foo", cinode, false) == E_SUCCESS);
		CHECK(dinode->Lookup(session, "foo", 0, &inode) == E_SUCCESS);
		
		dinode->Unlock(session);
}


#if 0
	TEST_FIXTURE(ClientFixture, TestLinkPublish1)
	{
		client::Inode*                inode;
		DirPnode<client::Session>*    rootpnode = new(session) DirPnode<client::Session>;
		PSuperBlock<client::Session>* psb = new(session) PSuperBlock<client::Session>;
		SuperBlock*                   sb;
		DirPnode<client::Session>*    dirpnode = new(session) DirPnode<client::Session>;
		DirPnode<client::Session>*    child;
		DirPnode<client::Session>*    child1;
		DirPnode<client::Session>*    child2;
		uint64_t                      ino;

		psb->root_ = (uint64_t) rootpnode;
		sb = new SuperBlock(session, psb);

		InitNames(session, dirpnode);

		DirInodeMutable* dinode = new DirInodeMutable(sb, dirpnode); 
		CHECK(dinode->Lookup(session, "foo", &inode)==0);
		CHECK(dinode->Lookup(session, "bar", &inode)==0);
		CHECK(dinode->Lookup(session, "doc", &inode)==0);
		
		child1 = new(session) DirPnode<client::Session>;
		CHECK(dinode->Link(session, "media", (uint64_t) child1, false)==0);
		CHECK(dinode->Lookup(session, "media", &inode) == 0);
	
		child2 = new(session) DirPnode<client::Session>;
		CHECK(dinode->Link(session, "music", (uint64_t) child2, false)==0);
		CHECK(dinode->Lookup(session, "music", &inode) == 0);
	
		CHECK(dinode->Publish(session) == 0);
		CHECK(dirpnode->Lookup(session, "media", &ino)==0);
		CHECK(ino == (uint64_t) child1);	
		CHECK(dirpnode->Lookup(session, "music", &ino)==0);
		CHECK(ino == (uint64_t) child2);	
	}

	
	TEST_FIXTURE(ClientFixture, TestLinkPublish2)
	{
		client::Inode*                inode;
		DirPnode<client::Session>*    rootpnode = new(session) DirPnode<client::Session>;
		PSuperBlock<client::Session>* psb = new(session) PSuperBlock<client::Session>;
		SuperBlock*                   sb;
		DirPnode<client::Session>*    dirpnode = new(session) DirPnode<client::Session>;
		DirPnode<client::Session>*    child;
		DirPnode<client::Session>*    child1;
		DirPnode<client::Session>*    child2;
		uint64_t                      ino;

		psb->root_ = (uint64_t) rootpnode;
		sb = new SuperBlock(session, psb);

		InitNames(session, dirpnode);

		DirInodeMutable* dinode = new DirInodeMutable(sb, dirpnode); 
		CHECK(dinode->Lookup(session, "foo", &inode)==0);
		CHECK(dinode->Lookup(session, "bar", &inode)==0);
		CHECK(dinode->Lookup(session, "doc", &inode)==0);
		
		CHECK(dinode->Unlink(session, "foo")==0);
		CHECK(dinode->Lookup(session, "foo", &inode) != 0);
		
		child1 = new(session) DirPnode<client::Session>;
		CHECK(dinode->Link(session, "media", (uint64_t) child1, false)==0);
		CHECK(dinode->Lookup(session, "media", &inode) == 0);
	
		CHECK(dinode->Publish(session) == 0);
		CHECK(dirpnode->Lookup(session, "media", &ino)==0);
		CHECK(ino == (uint64_t) child1);	
	}


	// checks that publish handles correctly the case when we unlink and 
	// then link the same name
	TEST_FIXTURE(ClientFixture, TestLinkPublish3)
	{
		client::Inode*                inode;
		DirPnode<client::Session>*    rootpnode = new(session) DirPnode<client::Session>;
		PSuperBlock<client::Session>* psb = new(session) PSuperBlock<client::Session>;
		SuperBlock*                   sb;
		DirPnode<client::Session>*    dirpnode = new(session) DirPnode<client::Session>;
		DirPnode<client::Session>*    child;
		DirPnode<client::Session>*    child1;
		DirPnode<client::Session>*    child2;
		uint64_t                      ino;
		
		psb->root_ = (uint64_t) rootpnode;
		sb = new SuperBlock(session, psb);

		InitNames(session, dirpnode);

		DirInodeMutable* dinode = new DirInodeMutable(sb, dirpnode); 
		CHECK(dinode->Lookup(session, "foo", &inode)==0);
		CHECK(dinode->Lookup(session, "bar", &inode)==0);
		CHECK(dinode->Lookup(session, "doc", &inode)==0);
		
		CHECK(dinode->Unlink(session, "foo")==0);
		CHECK(dinode->Lookup(session, "foo", &inode) != 0);
		
		child1 = new(session) DirPnode<client::Session>;
		CHECK(dinode->Link(session, "foo", (uint64_t) child1, false)==0);
		CHECK(dinode->Lookup(session, "foo", &inode) == 0);
	
		CHECK(dinode->Publish(session) == 0);
		CHECK(dirpnode->Lookup(session, "foo", &ino)==0);
		CHECK(ino == (uint64_t) child1);	
	}
#endif


}

