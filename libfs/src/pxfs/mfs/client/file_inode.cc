#include "pxfs/mfs/client/file_inode.h"
#include <stdint.h>
#include "common/util.h"
#include "pxfs/client/inode.h"
#include "pxfs/client/session.h"
#include "pxfs/client/const.h"
#include "pxfs/mfs/client/inode_factory.h"
#include "pxfs/mfs/client/dir_inode.h"
#include "pxfs/common/publisher.h"

namespace mfs {
namespace client {

// FIXME: Should Link/Unlink increment/decrement the link count as well? currently the caller 
// must do a separate call, which breaks encapsulation. 


int 
FileInode::Read(::client::Session* session, char* dst, uint64_t off, uint64_t n)
{
	return rw_ref()->proxy()->interface()->Read(session, dst, off, n);
}


int 
FileInode::Write(::client::Session* session, char* src, uint64_t off, uint64_t n)
{
	int ret;
	ret = rw_ref()->proxy()->interface()->Write(session, src, off, n);
	return ret;
}


int FileInode::nlink()
{
	dbg_log (DBG_INFO, "In inode %lx, nlink = %d\n", ino(), rw_ref()->proxy()->interface()->nlink());
	
	return rw_ref()->proxy()->interface()->nlink();
}


int FileInode::set_nlink(int nlink)
{
	dbg_log (DBG_INFO, "In inode %lx, set nlink = %d\n", ino(), nlink);
	
	return rw_ref()->proxy()->interface()->set_nlink(nlink);
}


int
FileInode::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	ssa::containers::client::ByteContainer::Proxy* cc_proxy;

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Lock(session, mode);
	}
	return E_SUCCESS;
}




int
FileInode::Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode)
{
	ssa::containers::client::ByteContainer::Proxy* cc_proxy;
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
FileInode::Unlock(::client::Session* session)
{
	ssa::containers::client::ByteContainer::Proxy* cc_proxy;

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Unlock(session);
	}
	return E_SUCCESS;
}
 

int 
FileInode::Sync(::client::Session* session)
{
	return rw_ref()->proxy()->interface()->vUpdate(session);
}


int 
FileInode::ioctl(::client::Session* session, int request, void* info)
{
	int ret = E_SUCCESS;
	switch (request)
	{
		case kIsEmpty: {
			bool isempty = (rw_ref()->proxy()->interface()->Size(session) == 0) ? false: true;
			*((bool *) info) = isempty;
		} break;
		case kSize: {
			uint64_t size = rw_ref()->proxy()->interface()->Size(session);
			*((bool *) info) = size;
		} break;
	}
	return ret;
}


} // namespace client
} // namespace mfs
