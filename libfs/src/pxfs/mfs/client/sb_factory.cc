#define  __CACHE_GUARD__

#include "pxfs/mfs/client/dir_inode.h"
#include "pxfs/mfs/client/inode_factory.h"
#include "pxfs/mfs/client/sb.h"
#include "osd/main/common/obj.h"

#include "pxfs/mfs/client/sb_factory.h"
#include "common/errno.h"
#include "pxfs/client/session.h"
#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"

namespace mfs {
namespace client {


SuperBlockFactory::SuperBlockFactory()
{
	
}


int 
SuperBlockFactory::Load(::client::Session* session, osd::common::ObjectId oid, 
                        ::client::SuperBlock** sbp)
{
	int                                ret;
	SuperBlock*                        sb; 
	::client::Inode*                   dip;
	osd::common::ObjectProxyReference* ref;
	osd::common::ObjectId              root_oid;

	if ((ret = session->omgr_->FindObject(session, oid, &ref)) == E_SUCCESS) {
		if (ref->owner()) {
			// the in-core superblock already exists; just return this and 
			// we are done
			sb = reinterpret_cast<SuperBlock*>(ref->owner());
			goto done;
		} else {
			sb = new SuperBlock(ref);
		}
	} else {
		sb = new SuperBlock(ref);
	}

	root_oid = sb->super_rw_ref_->proxy()->interface()->root(session);
	if ((ret = InodeFactory::LoadDirInode(session, root_oid, &dip)) < 0) {
		//FIXME: deallocate the allocated superblock and dirinode object
		return ret;
	}
	sb->root_ = dip;
done:	
	*sbp = sb;
	return E_SUCCESS;
}


int
SuperBlockFactory::Make(::client::Session* session, ::client::SuperBlock** sbp)
{
	int                                                ret;
	SuperBlock*                                        sb; 
	::client::Inode*                                   dip;
	osd::containers::client::SuperContainer::Object*   super_obj;
	osd::containers::client::NameContainer::Object*    root_obj;

	// first create the superblock object/proxy,
	// second create the directory inode objext/proxy and set the root 
	// of the superblock to point to the new directory inode.
	
	// superblock 
	// FIXME: allocate object through the storage manager
	if ((super_obj = osd::containers::client::SuperContainer::Object::Make(session)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = Load(session, super_obj->oid(), sbp)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}
	sb = static_cast<SuperBlock*>(*sbp);

	// root directory inode
	if ((root_obj = osd::containers::client::NameContainer::Object::Make(session)) == NULL) {
		return -E_NOMEM;
	}
	sb->super_rw_ref_->proxy()->interface()->set_root(session, root_obj->oid());
	if ((ret = InodeFactory::LoadDirInode(session, root_obj->oid(), &dip)) < 0) {
		//FIXME: deallocate the allocated superblock and dirinode object
		return ret;
	}
	sb->root_ = dip;
	return E_SUCCESS;
}


} // namespace client
} // namespace mfs
