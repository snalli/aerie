#include <string>
#include "common/errno.h"
#include "bcs/bcs.h"
#include "osd/main/server/osd.h"
#include "osd/main/server/sessionmgr.h"
#include "osd/containers/array/container.h"
#include "scm/pool/pool.h"
#include "kvfs/common/fs_protocol.h"
#include "kvfs/server/fs.h"
#include "kvfs/server/session.h"
#include "kvfs/server/server.h"
#include "kvfs/server/publisher.h"


namespace server {

typedef osd::containers::server::ArrayContainer<osd::common::ObjectId>::Object ObjectIdArray;

FileSystem::FileSystem(Ipc* ipc, StorageSystem* storage_system)
	: ipc_(ipc),
	  storage_system_(storage_system)
{
	pthread_mutex_init(&mutex_, NULL);
}


int
FileSystem::Init()
{
	int ret;
	if (ipc_) {
		if ((ret = ipc_handlers_.Register(this)) < 0) {
			return ret;
		}
	}
	Publisher::Init();
	return Publisher::Register(storage_system_);
}


int 
FileSystem::Make(const char* target, size_t nblocks, size_t block_size, int flags) 
{
	int                                              ret;
	void*                                            root_obj;
	int                                              root_obj_size;
	char*                                            ptr;
	osd::containers::server::SuperContainer::Object* super_obj;
	osd::containers::server::NameContainer::Object*  name_obj;
	ObjectIdArray*                                   array_obj;

	root_obj_size = sizeof(ObjectIdArray);
	root_obj_size += ARRAY_CONTAINER_SIZE * sizeof(osd::containers::server::NameContainer::Object);
	if ((ret = StorageSystem::Make(target, flags, root_obj_size, (void**) &super_obj, &root_obj)) < 0) {
		return ret;
	}
	array_obj = ObjectIdArray::Make((Session*) NULL, (char*) root_obj);
	for (int i=0; i<ARRAY_CONTAINER_SIZE; i++) {
		ptr = (char*) root_obj + sizeof(ObjectIdArray) + 
		      i * sizeof(osd::containers::server::NameContainer::Object);
		// pass NULL for the session as we need no journaling and storage allocator
		name_obj = osd::containers::server::NameContainer::Object::Make((Session*) NULL, ptr);
		array_obj->Write((Session*) NULL, i, name_obj->oid());
	}
	super_obj->set_root((Session*) NULL, array_obj->oid());

	return E_SUCCESS;
}


int 
FileSystem::Load(Ipc* ipc, const char* source, unsigned int flags, FileSystem** fsp)
{
	int            ret;
	StorageSystem* storage_system;
	FileSystem*    fs;

	if ((ret = StorageSystem::Load(ipc, source, flags, &storage_system)) < 0) {
		return ret;
	}
	if ((fs = new FileSystem(ipc, storage_system)) == NULL) {
		storage_system->Close();
		return -E_NOMEM;
	}
	if ((ret = fs->Init()) < 0) {
		return ret;
	}
	*fsp = fs;
	return E_SUCCESS;
}


int 
FileSystem::Mount(int clt, const char* source, unsigned int flags, 
                  FileSystemProtocol::MountReply& rep) 
{
	int                               ret;
	StorageSystemProtocol::MountReply ssrep;
	
	if ((ret = storage_system_->Mount(clt, source, flags, ssrep)) < 0) {
		return ret;
	}
	rep.desc_ = ssrep.desc_;
	return E_SUCCESS;
}


int 
FileSystem::IpcHandlers::Register(FileSystem* module)
{
	module_ = module;
	module_->ipc_->reg(::FileSystemProtocol::kMount, this, 
	                   &::server::FileSystem::IpcHandlers::Mount);

	return E_SUCCESS;
}


int
FileSystem::IpcHandlers::Mount(unsigned int clt, std::string source, 
                               unsigned int flags,
                               FileSystemProtocol::MountReply& rep)
{
	int ret;
	
	dbg_log (DBG_INFO, "Mount: %s\n", source.c_str());

	if ((ret = module_->Mount(clt, source.c_str(), flags, rep)) < 0) {
		return -ret;
	}
	return 0;
}


} // namespace server
