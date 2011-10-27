#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "tool/testfw/unittest.h"
//#include "mfs/client/dir_inode.h"
#include "common/errno.h"
#include "client.fixture.h"
#include "client/namespace.h"
#include "client/sb.h"
#include "client/inode.h"



class PseudoDirInode: public client::Inode {
public:
	typedef google::dense_hash_map<std::string, client::Inode*> EntryCache;
	
	PseudoDirInode(client::SuperBlock* sb);

	int Init(client::Session* session, client::InodeNumber ino) { assert(0); }
	int Open(client::Session* session, const char* path, int flags) { assert(0); }
	int Write(client::Session* session, char* src, uint64_t off, uint64_t n) { assert(0); }
	int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) { assert(0); }
	int Lookup(client::Session* session, const char* name, client::Inode** inode);
	int LookupFast(client::Session* session, const char* name, client::Inode* inode) { assert(0); }
	int Insert(client::Session* session, const char* name, client::Inode* inode) { assert(0); }
	int Link(client::Session* session, const char* name, client::Inode* inode, bool overwrite);
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) { assert(0); }
	int Unlink(client::Session* session, const char* name) { assert(0); }

	int Publish(client::Session* session) { assert(0); }

	client::SuperBlock* GetSuperBlock() { return sb_; }
	void SetSuperBlock(client::SuperBlock* sb) { sb_ = sb; }

private:
	client::SuperBlock* sb_;
	EntryCache          entries_;
};


PseudoDirInode::PseudoDirInode(client::SuperBlock* sb)
	: sb_(sb)
{ 
	entries_.set_empty_key("");
}

int
PseudoDirInode::Lookup(client::Session* session, const char* name, client::Inode** inode) 
{ 
	EntryCache::iterator  it;
	
	if ((it = entries_.find(name)) != entries_.end()) {
		*inode = it->second;
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
		root_ = new PseudoDirInode(this);
	}

	client::Inode* GetRootInode() { return root_; }
	client::Inode* CreateImmutableInode(int type) { assert(0); }
	int AllocInode(client::Session* session, int type, client::Inode** ipp) { assert(0); }
	int GetInode(client::InodeNumber ino, client::Inode** ipp) { assert(0); }
	int PutInode(client::Inode* ip) { assert(0); }
	client::Inode* WrapInode() { assert(0); }
	void* GetPSuperBlock() { assert(0); }

private:
	PseudoDirInode* root_;
};


SUITE(ClientNamespace)
{
	TEST_FIXTURE(ClientFixture, TestMount)
	{
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb = new PseudoSuperBlock();
		client::Inode*      dirinode = new PseudoDirInode(sb);
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
		
		CHECK(test_namespace->Nameiparent(session, "/A/B/../B/C", tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->GetRootInode());
		
		CHECK(test_namespace->Nameiparent(session, "/A/../A/B/C", tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->GetRootInode());
		
		CHECK(test_namespace->Nameiparent(session, "/A/./B/C", tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->GetRootInode());
		
		CHECK(test_namespace->Nameiparent(session, "/A/B/C", tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "C") == 0);
		CHECK(inode == sb->GetRootInode());
		
		// link /A/B/C to an inode
		client::Inode* dirinode1 = new PseudoDirInode(sb);
		CHECK(inode->Link(session, "C", dirinode1, false) == 0);

		CHECK(test_namespace->Nameiparent(session, "/A/B/C/D", tmpname, &inode) == 0);
		CHECK(strcmp(tmpname, "D") == 0);
		CHECK(inode == dirinode1);
	}


	TEST_FIXTURE(ClientFixture, TestNamei)
	{
		char                tmpname[128];
		client::Inode*      inode;
		client::NameSpace*  test_namespace = new client::NameSpace(NULL, 0, "TEST");
		client::SuperBlock* sb = new PseudoSuperBlock();
		client::Inode* dirinode1 = new PseudoDirInode(sb);
		inode = sb->GetRootInode();
		CHECK(inode->Link(session, "C", dirinode1, false) == 0);
		test_namespace->Init(session);
		
		CHECK(test_namespace->Mount(session, "/A/B", sb) == 0);
		
		CHECK(test_namespace->Namei(session, "/A/B/../B/C", &inode) == 0);
		CHECK(inode == dirinode1);
		
		CHECK(test_namespace->Namei(session, "/A/../A/B/C", &inode) == 0);
		CHECK(inode == dirinode1);
		
		CHECK(test_namespace->Namei(session, "/A/./B/C", &inode) == 0);
		CHECK(inode == dirinode1);
		
		CHECK(test_namespace->Namei(session, "/A/B/C", &inode) == 0);
		CHECK(inode == dirinode1);
		
		CHECK(test_namespace->Namei(session, "/A/B/../B/C", &inode) == 0);
		CHECK(inode == dirinode1);
	}

}
