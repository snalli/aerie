#include "mfs/server/fs_factory.h"
#include "common/errno.h"
#include "server/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/main/common/obj.h"


namespace mfs {
namespace server {


FileSystemFactory::FileSystemFactory()
{
	
}


int
FileSystemFactory::Make(::server::Session* session)
{
	int                                                ret;
	//dpo::containers::server::SuperContainer::Object*   super_obj;
	//dpo::containers::server::NameContainer::Object*    root_obj;

	// first create the superblock object/proxy,
	// second create the directory inode objext/proxy and set the root 
	// of the superblock to point to the new directory inode.

/*
	// superblock 
	// FIXME: allocate object through the storage manager
	if ((super_obj = new(session) dpo::containers::server::SuperContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = Load(session, super_obj->oid(), sbp)) < 0) {
		//FIXME: deallocate the allocated object
		return ret;
	}
	sb = static_cast<SuperBlock*>(*sbp);

	// root directory inode
	if ((root_obj = new(session) dpo::containers::server::NameContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	sb->super_rw_ref_->proxy()->interface()->set_root(session, root_obj->oid());
	if ((ret = InodeFactory::LoadDirInode(session, root_obj->oid(), &dip)) < 0) {
		//FIXME: deallocate the allocated superblock and dirinode object
		return ret;
	}
	sb->root_ = dip;
*/	
	return E_SUCCESS;
}


} // namespace server
} // namespace mfs
