#include "pxfs/mfs/server/dir_inode.h"
#include "pxfs/mfs/server/file_inode.h"
#include "pxfs/server/session.h"
#include "pxfs/server/const.h"
#include "osd/main/common/obj.h"
#include "common/errno.h"
#include "common/util.h"
#include <stdio.h>

namespace server {


int 
DirInode::Link(Session* session, const char* name, uint64_t ino)
{
	if (Inode::type(ino) != kFileInode) {
		return -1;
	}

	// assign parent of underlying object
	// set link count

	return E_SUCCESS;
}


int 
DirInode::Link(Session* session, const char* name, DirInode* child)
{
	obj_->Insert(session, name, child->oid());
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
	return E_SUCCESS;
}


int 
DirInode::Link(Session* session, const char* name, FileInode* child)
{
	if (str_is_dot(name) > 0) {
		dbg_log (DBG_INFO, "Validation failed: trying to link a file as %s\n", name);
		return -E_VRFY;
	}
	obj_->Insert(session, name, child->oid());
	child->obj()->set_parent(oid());
	child->obj()->set_nlink(child->obj()->nlink() + 1);
	return E_SUCCESS;
}


int 
DirInode::Unlink(Session* session, const char* name)
{
	int         ret;
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
			// if inode unreachable and no client has the file open, then deallocate the container
			// deallocating the container also deallocates any extents reachable from it
			if (nlink - 1 == 0) {
				FileInode::Free(session, fp);
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


} // namespace server
