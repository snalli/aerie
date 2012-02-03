#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "common/types.h"
#include "unit/fixture/client.fixture.h"
#include "dpo/base/common/obj.h"
#include "client/namespace.h"
#include "client/sb.h"
#include "client/inode.h"


class PseudoDirInode: public client::Inode {
public:
	typedef google::dense_hash_map<std::string, client::Inode*> EntryCache;
	
	PseudoDirInode();

	int Init(client::Session* session, InodeNumber ino) { assert(0); }
	int Open(client::Session* session, const char* path, int flags) { assert(0); }
	int Write(client::Session* session, char* src, uint64_t off, uint64_t n) { assert(0); }
	int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) { assert(0); }
	int Lookup(client::Session* session, const char* name, int flag, client::Inode** inode);
	int xLookup(client::Session* session, const char* name, int flag, client::Inode** inode) { assert(0); }
	int Insert(client::Session* session, const char* name, client::Inode* inode) { assert(0); }
	int Link(client::Session* session, const char* name, client::Inode* inode, bool overwrite);
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) { assert(0); }
	int Unlink(client::Session* session, const char* name) { assert(0); }
	
	int nlink() { return nlink_; }
	int set_nlink(int nlink) { nlink_ = nlink; return 0; }

	int Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode) { return 0; }
	int Lock(::client::Session* session, lock_protocol::Mode mode) { return 0; }
	int Unlock(::client::Session* session) { return 0; }
	int xOpenRO(client::Session* session) { assert(0); }

	int ioctl(::client::Session* session, int request, void* info) { return 0; }

private:
	EntryCache  entries_;
	int         nlink_;
};


PseudoDirInode::PseudoDirInode()
{ 
	entries_.set_empty_key("");
}

int
PseudoDirInode::Lookup(client::Session* session, const char* name, int flag, client::Inode** inode) 
{ 
	EntryCache::iterator  it;
	
	if ((it = entries_.find(name)) != entries_.end()) {
		*inode = it->second;
		(*inode)->Get(); // bump up the ref cnt
		return E_SUCCESS;
	}
	return -E_NOENT;
}


int 
PseudoDirInode::Link(client::Session* session, const char* name, client::Inode* inode, bool overwrite)
{
	EntryCache::iterator  it;
	
	if ((it = entries_.find(name)) != entries_.end()) {
		return -E_EXIST;
	}

	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, client::Inode*>(name, inode));
	assert(ret_pair.second == true);
	return -E_SUCCESS;
}


class PseudoSuperBlock: public client::SuperBlock {
public:
	PseudoSuperBlock()
	{
		root_ = new PseudoDirInode();
	}

	client::Inode* RootInode() { return root_; }
	dpo::common::ObjectId oid() { return dpo::common::ObjectId(0); }

private:
	client::Inode* root_;
};


SUITE(ClientNamespace)
{

	TEST_FIXTURE(ClientFixture, TestMount)
	{
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb = new PseudoSuperBlock();
		client::Inode*      dirinode = new PseudoDirInode();
		test_namespace->Init(session);
		
		CHECK(test_namespace->Mount(session, "/A/B", sb) == 0);
	}
	
	TEST_FIXTURE(ClientFixture, TestMultipleMount)
	{
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb1 = new PseudoSuperBlock();
		client::SuperBlock* sb2 = new PseudoSuperBlock();
		test_namespace->Init(session);
		
		CHECK(test_namespace->Mount(session, "/A/B", sb1) == 0);
		CHECK(test_namespace->Mount(session, "/A/B/C", sb2) != 0);
		CHECK(test_namespace->Mount(session, "/A/C/D", sb2) == 0);
	}


	TEST_FIXTURE(ClientFixture, TestNameiparent)
	{
		char                tmpname[128];
		client::Inode*      inode;
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb = new PseudoSuperBlock();
		test_namespace->Init(session);
		
		CHECK(test_namespace->Mount(session, "/A/B", sb) == 0);
		
		CHECK(test_namespace->Nameiparent(session, "/A/B/../B/C", lock_protocol::Mode::NL, tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->RootInode());
		inode->Put();
		
		CHECK(test_namespace->Nameiparent(session, "/A/../A/B/C", lock_protocol::Mode::NL, tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->RootInode());
		inode->Put();
		
		CHECK(test_namespace->Nameiparent(session, "/A/./B/C", lock_protocol::Mode::NL, tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->RootInode());
		inode->Put();
		
		CHECK(test_namespace->Nameiparent(session, "/A/B/C", lock_protocol::Mode::NL, tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->RootInode());
		inode->Put();
		
		// link /A/B/C to an inode
		client::Inode* dirinode1 = new PseudoDirInode();
		CHECK(inode->Link(session, "C", dirinode1, false) == 0);

		CHECK(test_namespace->Nameiparent(session, "/A/B/C/D", lock_protocol::Mode::NL, tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "D") == 0);
		CHECK(inode == dirinode1);
		inode->Put();
	}


	TEST_FIXTURE(ClientFixture, TestNamei)
	{
		char                tmpname[128];
		client::Inode*      inode;
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb = new PseudoSuperBlock();
		client::Inode*      dirinode1 = new PseudoDirInode();
		inode = sb->RootInode();
		CHECK(inode->Link(session, "C", dirinode1, false) == 0);
		test_namespace->Init(session);
		
		CHECK(test_namespace->Mount(session, "/A/B", sb) == 0);
		
		CHECK(test_namespace->Namei(session, "/A/B/../B/C", lock_protocol::Mode::NL, &inode) == 0);
		CHECK(inode == dirinode1);
		inode->Put();
		
		CHECK(test_namespace->Namei(session, "/A/../A/B/C", lock_protocol::Mode::NL, &inode) == 0);
		CHECK(inode == dirinode1);
		inode->Put();
		
		CHECK(test_namespace->Namei(session, "/A/./B/C", lock_protocol::Mode::NL, &inode) == 0);
		CHECK(inode == dirinode1);
		inode->Put();
		
		CHECK(test_namespace->Namei(session, "/A/B/C", lock_protocol::Mode::NL, &inode) == 0);
		CHECK(inode == dirinode1);
		inode->Put();
		
		CHECK(test_namespace->Namei(session, "/A/B/../B/C", lock_protocol::Mode::NL, &inode) == 0);
		CHECK(inode == dirinode1);
		inode->Put();
	}


	TEST_FIXTURE(ClientFixture, TestNameiNotExists)
	{
		char                tmpname[128];
		client::Inode*      inode;
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb = new PseudoSuperBlock();
		client::Inode*      dirinode1 = new PseudoDirInode();
		inode = sb->RootInode();
		test_namespace->Init(session);
		
		CHECK(test_namespace->Mount(session, "/A/B", sb) == 0);
		
		CHECK(test_namespace->Namei(session, "/A/B/../B/C", lock_protocol::Mode::NL, &inode) != 0);
		
		CHECK(test_namespace->Namei(session, "/A/../A/B/C", lock_protocol::Mode::NL, &inode) != 0);
		
		CHECK(test_namespace->Namei(session, "/A/./B/C", lock_protocol::Mode::NL, &inode) != 0);
		
		CHECK(test_namespace->Namei(session, "/A/B/C", lock_protocol::Mode::NL, &inode) != 0);
		
		CHECK(test_namespace->Namei(session, "/A/B/../B/C", lock_protocol::Mode::NL, &inode) != 0);
	}


}
