#include "rxfs/client/dir_inode.h"
#include "rxfs/client/file_inode.h"
#include "rxfs/client/session.h"
#include "rxfs/client/const.h"
#include "osd/main/common/obj.h"
#include "common/errno.h"
#include "common/util.h"
#include <stdio.h>

namespace client {


int 
DirInode::Link(Session* session, const char* name, uint64_t ino)
{
#if 0
	if (Inode::type(ino) != kFileInode) {
		return -1;
	}

	// assign parent of underlying object
	// set link count
#endif
	return E_SUCCESS;
}


int 
DirInode::Link(Session* session, const char* name, DirInode* child)
{
	int ret;
#if 0
	if ((ret = obj_->Insert(session, name, child->oid())) < 0) {
		dbg_log (DBG_ERROR, "Failed: trying to link a file as %s\n", name);
		return ret;
	}
	switch (str_is_dot(name)) {
		case 1: // .
			break;
		case 2: // ..
			// child is really our parent so don't set parent
			child->obj()->set_nlink(child->obj()->nlink() + 1);
			break;
		default:
			child->obj()->set_parent(oid());
			child->obj()->set_nlink(child->obj()->nlink() + 1);
	}
#endif
	return E_SUCCESS;
}


int 
DirInode::Link(Session* session, const char* name, FileInode* child)
{
	int ret;
#if 0
	if (str_is_dot(name) > 0) {
		dbg_log (DBG_INFO, "Failed: trying to link a file as %s\n", name);
		return -E_VRFY;
	}
	if ((ret = obj_->Insert(session, name, child->oid())) < 0) {
		dbg_log (DBG_ERROR, "Failed: trying to link a file as %s\n", name);
		return ret;
	}
	child->obj()->set_parent(oid());
	child->obj()->set_nlink(child->obj()->nlink() + 1);
#endif
	return E_SUCCESS;
}


int 
DirInode::Unlink(Session* session, const char* name)
{
	int         ret;
#if 0
	int         nlink;
	FileInode   finode;
	DirInode    dinode;
	FileInode*  fp;
	DirInode*   dp;
	InodeNumber child_ino;

	if ((ret = Lookup(session, name, &child_ino)) < 0) { return ret; }
	switch (Inode::type(child_ino)) {
		case kFileInode:
			fp = FileInode::Load(session, child_ino, &finode);
			// it's possible the new nlink to be 1 but the parent hint be zero.
			// this case will later be detected by a client and provide us with a new 
			// parent hint which we validate.
			obj()->Erase(session, name);
			fp->obj()->set_parent(osd::common::ObjectId(0));
			nlink = fp->obj()->nlink(); 
			fp->obj()->set_nlink(nlink - 1);
			// deallocate inode if unreachable and no client has the file open
			if (nlink - 1 == 0) {
				//TODO
			}
			ret = E_SUCCESS;
			break;
		case kDirInode:
			dp = DirInode::Load(session, child_ino, &dinode);
			if (dp->obj()->Size(session) > 2) {
				ret = -E_VRFY;
				break;
			}
			obj()->Erase(session, name);
			dp->obj()->set_parent(osd::common::ObjectId(0));
			nlink = dp->obj()->nlink();
			dp->obj()->set_nlink(nlink - 1); // for forward link
			obj()->set_nlink(obj()->nlink() - 1); // for child's backward link ..
			// deallocate inode if unreachable 
			// (if nlink becomes 1 as the directory still contains .. )
			if (nlink - 1 == 1) {
				//TODO
			}
			ret = E_SUCCESS;
			break;
	}
#endif
	return ret;
}


int 
DirInode::Lookup(Session* session, const char* name, InodeNumber* ino)
{
	int                   ret;
	osd::common::ObjectId child_oid;

	if ((ret = obj_->Find(session, name, &child_oid)) < 0) { return ret; }
	*ino = child_oid.u64();
	return E_SUCCESS;
}


} // namespace client
