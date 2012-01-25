#include "mfs/client/dir_inode.h"
#include <stdint.h>
#include "client/backend.h"
#include "client/inode.h"
#include "client/session.h"
#include "mfs/client/inode_factory.h"

namespace mfs {
namespace client {


//TODO thread safety: acquire/release mutex to protect volatile metadata/data structures
//     persistent is protected by the lock. the caller should have already acquire the lock


int 
DirInode::Lookup(::client::Session* session, const char* name, ::client::Inode** ipp) 
{
	int                   ret;
	::client::Inode*      ip;
	dpo::common::ObjectId oid;
	
	dbg_log (DBG_INFO, "Lookup %s in inode %lx\n", name, ino());

	assert(ref_ != NULL);

	if ((ret = rw_ref()->obj()->interface()->Find(session, name, &oid)) != E_SUCCESS) {
		return ret;
	}
	if ((ret = InodeFactory::LoadInode(session, oid, &ip)) != E_SUCCESS) {
		return ret;
	}
	ip->Get();
    *ipp = ip;
	return E_SUCCESS;
}


int 
DirInode::Link(::client::Session* session, const char* name, ::client::Inode* ip, 
               bool overwrite)
{
	uint64_t ino = ip->ino();
	
	//FIXME: fix link count
	return Link(session, name, ino, overwrite);
}


// FIXME: How do we shadow . and ..??? In the EntryCache? It should work.
// FIXME: Should Link increment the link count as well? currently the caller 
// must do a separate call but this breaks encapsulation. 
int 
DirInode::Link(::client::Session* session, const char* name, uint64_t ino, 
               bool overwrite)
{
// FIXME: INODE
/*
	assert(overwrite == false);
	EntryCache::iterator   it;
	int                    ret;

	dbg_log (DBG_INFO, "In pnode %lu, linking: %s -> %lu\n", ino_, name, ino);	

	if (name[0] == '\0') {
		return -1;
	}

	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.first == true) {
			// name exists in the volatile cache
			ino = it->second.second;
			return -E_EXIST;
		} else {
			// found a negative entry indicating absence due to a previous 
			// unlink. just overwrite.
			it->second.first = true;
			it->second.second = ino;
			return E_SUCCESS;
		}
	}

	//FIXME
	//if ((ret = ObjectProxy::subject()->Lookup(session, name, &ino)) == 0) {
	//	// name exists in the persistent structure
	//	return -E_EXIST;
	//}
	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, std::pair<bool, uint64_t> >(name, std::pair<bool, uint64_t>(true, ino)));
	assert(ret_pair.second == true);
	printf("DirInode::Link (%s): DONE\n", name);
*/
	return 0;
}


int 
DirInode::Unlink(::client::Session* session, const char* name)  
{

// FIXME: INODE
/*
	EntryCache::iterator   it;
	int                    ret;
	uint64_t               ino;

	if (name[0] == '\0') {
		return -1;
	}

	if ((it = entries_.find(name)) != entries_.end()) {
		if (it->second.first == true) {
			// name exists
			it->second.first = false;
			return E_SUCCESS;
		} else {
			// a negative entry indicating absence due to a previous unlink
			return -E_NOENT;
		}
	}
	
	// FIXME
	//if ((ret = ObjectProxy::subject()->Lookup(session, name, &ino)) != 0) {
	//	// name does not exist in the persistent structure
	//	return -E_EXIST;
	//}
	// add a negative directory entry when removing a directory entry from 
	// the persistent data structure
	std::pair<EntryCache::iterator, bool> ret_pair = entries_.insert(std::pair<std::string, std::pair<bool, uint64_t> >(name, std::pair<bool, uint64_t>(false, ino)));
	neg_entries_count_++;
	assert(ret_pair.second == true);
*/
	return E_SUCCESS;
}


/*
//  List all directory entries of a directory
//     This requires coordinating with the immutable directory inode. Simply
//     taking the union of the two inodes is not correct as some entries 
//     that appear in the persistent inode may have been removed and appear
//     as negative entries in the volatile cache. One way is for each entry 
//     in the persistent inode to check whether there is a corresponding negative
//     entry in the volatile cache. But this sounds like an overkill. 
//     Perhaps we need a combination of a bloom filter of just the negative entries 
//     and a counter of the negative entries. As deletes are rare, the bloom filter
//     should quickly provide an answer.

int 
DirInode::Readdir()
{

}
*/


int 
DirInode::Publish(::client::Session* session)
{
// FIXME: INODE
/*
	// FIXME: Currently we publish by simply doing the updates in-place. 
	// Normally this must be done via the trusted server using the journal 

	EntryCache::iterator  it;
	int                   ret;

	printf("PUBLISH\n");
	for (it = entries_.begin(); it != entries_.end(); it++) {
		printf("PUBLISH: entry\n");
		if (it->second.first == true) {
			// normal entry -- link
			// FIXME: if a link followed an unlink then the entry is 
			// marked as valid (true). Adding this entry to the persistent 
			// directory without removing the previous one is incorrect.
			// however, currently we don't have a way to detect this case.
			// consider modifying the cache.
			// this case won't be a problem  when doing the publish through the server
			// because there we replay the journal which has the unlink operation.
			// so don't worry for now.
			// TEST TestLinkPublish3 checks this case
			printf("Publish: %s->%lu\n", it->first.c_str(), it->second.second);
			// FIXME
			//if ((ret = ObjectProxy::subject()->Link(session, it->first.c_str(), it->second.second)) != 0) {
			//	return ret;
			//}
		} else {
			// negative entry -- unlink
			//FIXME
			//if ((ret = ObjectProxy::subject()->Unlink(session, it->first.c_str())) != 0) {
			//	return ret;
			//}
		}
	}
	printf("inode %lu: nlink_ = %d\n", ino_, nlink_);
	//FIXME ObjectProxy::subject()->set_nlink(nlink_);
*/
	return 0;
}


int DirInode::nlink()
{
	return nlink_;
}


int DirInode::set_nlink(int nlink)
{
	nlink_ = nlink; 
	return 0;
}



} // namespace client
} // namespace mfs
