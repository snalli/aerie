#include "mfs/client/sb_factory.h"
#include "common/errno.h"
#include "client/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/base/common/obj.h"
#include "mfs/client/sb.h"
#include "mfs/client/dir_inode.h"
#include "mfs/client/inode_factory.h"

namespace mfs {
namespace client {


SuperBlockFactory::SuperBlockFactory()
{
	
}


int 
SuperBlockFactory::Load(::client::Session* session, dpo::common::ObjectId oid, 
                        ::client::SuperBlock** sbp)
{
	int                                ret;
	SuperBlock*                        sb; 
	::client::Inode*                   dip;
	dpo::common::ObjectProxyReference* ref;
	dpo::common::ObjectId              root_oid;

	if ((ret = session->omgr_->FindObject(oid, &ref)) == E_SUCCESS) {
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
	dpo::containers::client::SuperContainer::Object*   super_obj;
	dpo::containers::client::NameContainer::Object*    root_obj;

	// first create the superblock object/proxy,
	// second create the directory inode objext/proxy and set the root 
	// of the superblock to point to the new directory inode.
	
	// superblock 
	// FIXME: allocate object through the storage manager
	if ((super_obj = new(session) dpo::containers::client::SuperContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = Load(session, super_obj->oid(), sbp)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}
	sb = static_cast<SuperBlock*>(*sbp);

	// root directory inode
	if ((root_obj = new(session) dpo::containers::client::NameContainer::Object) == NULL) {
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
