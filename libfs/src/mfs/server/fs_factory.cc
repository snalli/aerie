#include "mfs/server/fs_factory.h"
#include "common/errno.h"
#include "server/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/containers/set/container.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/server/salloc.h"
#include "sal/pool/pool.h"
#include "mfs/server/fs.h"
#include "sal/const.h"



namespace mfs {
namespace server {


FileSystemFactory::FileSystemFactory()
{
	
}


int
FileSystemFactory::Make(StoragePool* pool, size_t nblocks, size_t block_size, int flags)
{
	dpo::containers::server::SuperContainer::Object*                      super_obj;
	dpo::containers::server::NameContainer::Object*                       root_obj;
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* set_obj;
	int                                                                   ret;
	char*                                                                 b;
	char*                                                                 buffer;
	::server::Session*                                                    session = NULL; // we need no journaling and storage allocator
	size_t                                                                master_extent_size;

	// 1) create the superblock object/proxy,
	// 2) create the directory inode objext/proxy and set the root 
	//    of the superblock to point to the new directory inode.
	
	master_extent_size = 0;
	master_extent_size += sizeof(dpo::containers::server::NameContainer::Object);
	master_extent_size += sizeof(dpo::containers::server::SuperContainer::Object);
	master_extent_size += sizeof(dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object);
	if ((ret = pool->AllocateExtent(master_extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	pool->set_root((void*) buffer);

	// superblock 
	b = buffer;
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
	super_obj->set_freelist(session, set_obj->oid());
	b += sizeof(dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object);
	assert(b < buffer + master_extent_size + 1); 
	return E_SUCCESS;
}


int
FileSystemFactory::Load(StoragePool* pool, int flags, ::server::FileSystem** filesystem)
{
	dpo::containers::server::SuperContainer::Object*                      super_obj;
	dpo::containers::server::NameContainer::Object*                       root_obj;
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* set_obj;
	void*                                                                 b;
	::server::Session*                                                    session = NULL; // we need no journaling and storage allocator
	
	if ((b = pool->root()) == 0) {
		return -E_NOENT;
	}

	if ((super_obj = dpo::containers::server::SuperContainer::Object::Load(b)) == NULL) {
		return -E_NOMEM;
	}
	dpo::common::ObjectId root_oid = super_obj->root(session);
	dpo::common::ObjectId freelist_oid = super_obj->freelist(session);

	if ((*filesystem = new FileSystem(super_obj->oid())) == NULL) {
		return -E_NOMEM;
	}
	return E_SUCCESS;
}


} // namespace server
} // namespace mfs
