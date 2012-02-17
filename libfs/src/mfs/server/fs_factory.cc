#include "mfs/server/fs_factory.h"
#include "common/errno.h"
#include "server/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/server/smgr.h"


namespace mfs {
namespace server {


FileSystemFactory::FileSystemFactory()
{
	
}


int
FileSystemFactory::Make(::server::Session* session, dpo::common::ObjectId* oid)
{
	int                                                ret;
	dpo::containers::server::SuperContainer::Object*   super_obj;
	dpo::containers::server::NameContainer::Object*    root_obj;

	// first create the superblock object/proxy,
	// second create the directory inode objext/proxy and set the root 
	// of the superblock to point to the new directory inode.

	// superblock 
	// FIXME: allocate object through the storage manager
	if ((super_obj = new(session) dpo::containers::server::SuperContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	// root directory inode
	if ((root_obj = new(session) dpo::containers::server::NameContainer::Object) == NULL) {
		return -E_NOMEM;
	}
	super_obj->set_root(session, root_obj->oid());
	*oid = super_obj->oid();
	return E_SUCCESS;
}


} // namespace server
} // namespace mfs
