#include "mfs/server/fs_factory.h"
#include "common/errno.h"
#include "server/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/containers/set/container.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/server/smgr.h"


namespace mfs {
namespace server {


FileSystemFactory::FileSystemFactory()
{
	
}


int
FileSystemFactory::Make(::server::Session* session, void* partition, 
                        dpo::common::ObjectId* oid)
{
	int                                                                   ret;
	dpo::containers::server::SuperContainer::Object*                      super_obj;
	dpo::containers::server::NameContainer::Object*                       root_obj;
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* set_obj;

	// first create the superblock object/proxy,
	// second create the directory inode objext/proxy and set the root 
	// of the superblock to point to the new directory inode.

	// superblock 
	char* b = (char*) partition;
	if ((super_obj = dpo::containers::server::SuperContainer::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	// root directory inode
	b += sizeof(dpo::containers::server::SuperContainer::Object);
	if ((root_obj = dpo::containers::server::NameContainer::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	super_obj->set_root(session, root_obj->oid());
	// storage container
	b += sizeof(dpo::containers::server::NameContainer::Object);
	if ((set_obj = dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	//FIXME
	//session->smgr()->RegisterStorageContainer(set_obj->oid());
	super_obj->set_freelist(session, set_obj->oid());
	*oid = super_obj->oid();
	return E_SUCCESS;
}


} // namespace server
} // namespace mfs
