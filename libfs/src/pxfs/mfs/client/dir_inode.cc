#include "pxfs/mfs/client/dir_inode.h"
#include <stdint.h>
#include "common/util.h"
#include "pxfs/client/backend.h"
#include "pxfs/client/inode.h"
#include "pxfs/client/session.h"
#include "pxfs/client/const.h"
#include "pxfs/mfs/client/inode_factory.h"
#include "pxfs/common/publisher.h"

namespace mfs {
namespace client {

// FIXME: Should Link/Unlink increment/decrement the link count as well? currently the caller 
// must do a separate call, which breaks encapsulation. 

int 
DirInode::Lookup(::client::Session* session, const char* name, int flags, ::client::Inode** ipp) 
{
	int                   ret;
	::client::Inode*      ip;
	ssa::common::ObjectId oid;
	
	dbg_log (DBG_INFO, "Lookup %s in inode %lx\n", name, ino());

	assert(ref_ != NULL);

	// special case: requesting parent (name=..) and parent_ points to a pseudo-inode
	if (parent_ && str_is_dot(name) == 2) {
		ip = parent_;
		goto done;
	}

	if ((ret = rw_ref()->proxy()->interface()->Find(session, name, &oid)) != E_SUCCESS) {
		return ret;
	}
	if ((ret = InodeFactory::LoadInode(session, oid, &ip)) != E_SUCCESS) {
		return ret;
	}

done:
	ip->Get();
    *ipp = ip;
	return E_SUCCESS;
}


int 
DirInode::xLookup(::client::Session* session, const char* name, int flags, ::client::Inode** ipp) 
{
	int                   ret;
	::client::Inode*      ip;
	ssa::common::ObjectId oid;
	
	dbg_log (DBG_INFO, "xLookup %s in inode %lx\n", name, ino());

	assert(ref_ != NULL);

	// special case: requesting parent (name=..) and parent_ points to a pseudo-inode
	if (parent_ && str_is_dot(name) == 2) {
		ip = parent_;
		goto done;
	}

	if ((ret = rw_ref()->proxy()->xinterface()->Find(session, name, &oid)) != E_SUCCESS) {
		return ret;
	}
	if ((ret = InodeFactory::LoadInode(session, oid, &ip)) != E_SUCCESS) {
		return ret;
	}

done:
	ip->Get();
    *ipp = ip;
	return E_SUCCESS;
}



int 
DirInode::Link(::client::Session* session, const char* name, ::client::Inode* ip, 
               bool overwrite)
{
	int ret; 

	dbg_log (DBG_INFO, "In inode %lx, link %s to inode %lx\n", ino(), name, ip->ino());

	session->journal() << Publisher::Messages::LogicalOperation::Link(ino(), name, ip->ino());

	// special case: if inode oid is zero then we link to a pseudo-inode. 
	// keep this link in the in-core state parent_
	if (ip->oid() == ssa::common::ObjectId(0)) {
		pthread_mutex_lock(&mutex_);
		parent_ = ip;
		pthread_mutex_unlock(&mutex_);
		return E_SUCCESS;
	}

	if ((ret = rw_ref()->proxy()->interface()->Insert(session, name, ip->oid())) != E_SUCCESS) {
		return ret;
	}
	return E_SUCCESS;
}


int 
DirInode::Unlink(::client::Session* session, const char* name)  
{
	int ret; 

	dbg_log (DBG_INFO, "In inode %lx, unlink %s\n", ino(), name);

	if ((ret = rw_ref()->proxy()->interface()->Erase(session, name)) != E_SUCCESS) {
		return ret;
	}
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


int DirInode::nlink()
{
	dbg_log (DBG_INFO, "In inode %lx, nlink = %d\n", ino(), rw_ref()->proxy()->interface()->nlink());
	
	return rw_ref()->proxy()->interface()->nlink();
}


int DirInode::set_nlink(int nlink)
{
	dbg_log (DBG_INFO, "In inode %lx, set nlink = %d\n", ino(), nlink);
	
	return rw_ref()->proxy()->interface()->set_nlink(nlink);
}


int
DirInode::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	ssa::containers::client::NameContainer::Proxy* cc_proxy;

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Lock(session, mode);
	}
	return E_SUCCESS;
}


int
DirInode::Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode)
{
	ssa::containers::client::NameContainer::Proxy* cc_proxy;
	ssa::containers::client::NameContainer::Proxy* cc_proxy_parent;

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		if (parent_inode->ref_) {
			// the parent_inode must always be of DirInode type, 
			// otherwise we can't do hierarchical locking right?
			DirInode* parent_dir_inode = dynamic_cast<DirInode*>(parent_inode);
			cc_proxy_parent = parent_dir_inode->rw_ref()->proxy();	
			return cc_proxy->Lock(session, cc_proxy_parent, mode);
		} else {
			return cc_proxy->Lock(session, mode);
		}
	}

	return E_SUCCESS;
}


int
DirInode::Unlock(::client::Session* session)
{
	ssa::containers::client::NameContainer::Proxy* cc_proxy;

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Unlock(session);
	}
	return E_SUCCESS;
}
 

int 
DirInode::xOpenRO(::client::Session* session)
{
	rw_ref()->proxy()->xOpenRO();
	return E_SUCCESS;
}


int 
DirInode::Sync(::client::Session* session)
{
	return rw_ref()->proxy()->interface()->vUpdate(session);
}


int 
DirInode::ioctl(::client::Session* session, int request, void* info)
{
	int ret = E_SUCCESS;
	switch (request)
	{
		case 1: // is empty?
			// empty if only entries is self (.) and parent (..)
			bool isempty = (rw_ref()->proxy()->interface()->Size(session) > 2) ? false: true;
			*((bool *) info) = isempty;
			break;
	}
	return ret;
}


} // namespace client
} // namespace mfs
