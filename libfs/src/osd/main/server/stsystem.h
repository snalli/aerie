#ifndef __STAMNOS_OSD_SERVER_STORAGE_SYSTEM_H
#define __STAMNOS_OSD_SERVER_STORAGE_SYSTEM_H

#include "osd/main/server/sessionmgr.h"
#include "bcs/bcs.h"
#include "scm/pool/pool.h"
#include "osd/main/server/osd-opaque.h"
#include "osd/main/common/stsystem.h"

#include "osd/main/server/osd.h"
#include "common/errno.h"
#include "bcs/bcs.h"
#include "osd/main/server/salloc.h"
#include "osd/main/server/hlckmgr.h"
#include "osd/main/server/registry.h"
#include "osd/main/server/shbuf.h"
#include "osd/main/server/session.h"
#include "osd/main/server/publisher.h"
#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "osd/containers/set/container.h"
#include "scm/pool/pool.h"
#include "scm/const.h"

#include <iostream>

namespace osd {
namespace server {


/** 
 * \brief Encapsulates the OSD components composing a storage system 
 */
class StorageSystem {
public:
	StorageSystem(::server::Ipc* ipc, StoragePool* pool)
		: ipc_(ipc),
		  salloc_(NULL),
		  pool_(pool),
		  can_commit_suicide_(false)
	{ 
		pthread_mutex_init(&mutex_, NULL);
	}

	osd::cc::server::HLockManager* hlckmgr() { return hlckmgr_; }
	osd::cc::server::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	Registry* registry() { return registry_; }
	Publisher* publisher() { return publisher_; }
	osd::containers::server::SuperContainer::Object* super_obj() { return super_obj_; }
	StoragePool* pool() { return pool_; }
	StorageSystemDescriptor Descriptor(OsdSession* session)
	{
		return StorageSystemDescriptor(super_obj_->oid(), 
									   session->shbuf_->Descriptor());
	}

protected:
	pthread_mutex_t                                   mutex_;
	::server::Ipc*                                    ipc_;
	osd::cc::server::HLockManager*                    hlckmgr_;
	osd::cc::server::LockManager*                     lckmgr_;
	StorageAllocator*                                 salloc_;
	Registry*                                         registry_;
	StoragePool*                                      pool_;
	Publisher*                                        publisher_;
	osd::containers::server::SuperContainer::Object*  super_obj_;
	bool                                              can_commit_suicide_;
};


template<typename Session>
class StorageSystemT: public StorageSystem {
public:
	// factory methods
	static int Load(::server::Ipc* ipc, const char* source, unsigned int flags, StorageSystemT** storage_system_ptr);
	static int Make(const char* target, unsigned int flags);
	
	StorageSystemT(::server::Ipc* ipc, StoragePool* pool)
		: StorageSystem(ipc, pool)
	{ }

	~StorageSystemT();

	int Init();
	int Close();

	int Mount(int clt, const char* source, unsigned int flags, StorageSystemProtocol::MountReply& rep);

	class IpcHandlers {
	public:
		int Register(StorageSystemT* module);
		int Mount(unsigned int clt, std::string source, unsigned int flags, StorageSystemProtocol::MountReply& rep);

	private:
		StorageSystemT* module_;
	};

private:
	SessionManager<Session, StorageSystemT>* sessionmgr_;
	IpcHandlers                              ipc_handlers_;
};


template<typename Session>
int
StorageSystemT<Session>::Init()
{
	int ret;

	if (ipc_) {
		if ((ret = ipc_handlers_.Register(this)) < 0) {
			return ret;
		}
	}
	if ((hlckmgr_ = new ::osd::cc::server::HLockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	hlckmgr_->Init();
	if ((ret = StorageAllocator::Load(ipc_, pool_, &salloc_)) < 0) {
		delete hlckmgr_;
		return ret;
	}
	if ((registry_ = new Registry(ipc_)) == NULL) {
		salloc_->Close();
		delete hlckmgr_;
	}
	registry_->Init();
	if ((sessionmgr_ = new SessionManager<Session, StorageSystemT>(ipc_, this)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = sessionmgr_->Init() == E_SUCCESS) < 0) {
		return ret;
	}
	if ((publisher_ = new Publisher(ipc_)) == NULL) {
		return ret;
	}
	if ((ret = publisher_->Init() == E_SUCCESS) < 0) {
		return ret;
	}
	if ((ret = ipc_->shbuf_manager()->RegisterSharedBufferType("OsdSharedBuffer", OsdSharedBuffer::Make)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


template<typename Session>
int
StorageSystemT<Session>::Load(::server::Ipc* ipc, const char* source, 
                              unsigned int flags, StorageSystemT** storage_system_ptr)
{
	int                ret;
	void*              b;
	StorageSystemT*    storage_system;	
	StoragePool*       pool;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(server_storagesystem), 
	        "Load storage system %s\n", source);

	if ((ret = StoragePool::Open(source, &pool)) < 0) {
		return ret;
	}
	if ((b = pool->root()) == 0) {
		StoragePool::Close(pool);
		return -E_NOENT;
	}
	if ((storage_system = new StorageSystemT(ipc, pool)) == NULL) {
		StoragePool::Close(pool);
		return -E_NOMEM;
	}
	if ((storage_system->super_obj_ = osd::containers::server::SuperContainer::Object::Load(b)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = storage_system->Init()) < 0) {
		return ret;
	}
	storage_system->can_commit_suicide_ = true;
	*storage_system_ptr = storage_system;

	return E_SUCCESS;
}


/**
 * \brief Creates a storage system in the pool
 */
template<typename Session>
int 
StorageSystemT<Session>::Make(const char* target, unsigned int flags)
{
	osd::containers::server::SuperContainer::Object*                      super_obj;
	osd::containers::server::NameContainer::Object*                       root_obj;
	osd::containers::server::SetContainer<osd::common::ObjectId>::Object* set_obj;
	int                                                                   ret;
	char*                                                                 b;
	char*                                                                 buffer;
	OsdSession*                                                           session = NULL; // we need no journaling and storage allocator
	size_t                                                                master_extent_size;
	StoragePool*                                                          pool;
	
	if ((ret = StoragePool::Open(target, &pool)) < 0) {
		return ret;
	}
	
	// 1) create the superblock object/proxy,
	// 2) create the directory inode objext/proxy and set the root 
	//    of the superblock to point to the new directory inode.
	
	master_extent_size = 0;
	master_extent_size += sizeof(osd::containers::server::NameContainer::Object);
	master_extent_size += sizeof(osd::containers::server::SuperContainer::Object);
	master_extent_size += sizeof(osd::containers::server::SetContainer<osd::common::ObjectId>::Object);
	if ((ret = pool->AllocateExtent(master_extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	pool->set_root((void*) buffer);

	// superblock 
	b = buffer;
	if ((super_obj = osd::containers::server::SuperContainer::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	// root directory inode
	b += sizeof(osd::containers::server::SuperContainer::Object);
	if ((root_obj = osd::containers::server::NameContainer::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	super_obj->set_root(session, root_obj->oid());
	// storage container
	b += sizeof(osd::containers::server::NameContainer::Object);
	if ((set_obj = osd::containers::server::SetContainer<osd::common::ObjectId>::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	super_obj->set_freelist(session, set_obj->oid());
	b += sizeof(osd::containers::server::SetContainer<osd::common::ObjectId>::Object);
	assert(b < buffer + master_extent_size + 1); 
	return E_SUCCESS;
}


template<typename Session>
int
StorageSystemT<Session>::Close()
{
	int ret;

	if ((ret = StoragePool::Close(pool_)) < 0) {
		return ret;
	}
	if (can_commit_suicide_) {
		delete this;
	}
	return E_SUCCESS;
}


template<typename Session>
StorageSystemT<Session>::~StorageSystemT()
{
	delete registry_;
	delete salloc_;
	delete hlckmgr_;
}


template<typename Session>
int 
StorageSystemT<Session>::Mount(int clt, const char* source, unsigned int flags, 
                               StorageSystemProtocol::MountReply& rep) 
{
	int                 ret;
	Session*            session;

	pthread_mutex_lock(&mutex_);
	if ((ret = sessionmgr_->Create(clt, &session)) < 0) {
		return -ret;
	}
	rep.desc_ = Descriptor(session);
	ret = E_SUCCESS;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


template<typename Session>
int 
StorageSystemT<Session>::IpcHandlers::Register(StorageSystemT* module)
{
	module_ = module;
	module_->ipc_->reg(::StorageSystemProtocol::kMount, this, 
	                   &::osd::server::StorageSystemT<Session>::IpcHandlers::Mount);
	return E_SUCCESS;
}


template<typename Session>
int
StorageSystemT<Session>::IpcHandlers::Mount(unsigned int clt, std::string source, 
                                            unsigned int flags, StorageSystemProtocol::MountReply& rep)
{
	int ret;
	
	if ((ret = module_->Mount(clt, source.c_str(), flags, rep)) < 0) {
		return -ret;
	}
	return 0;
}


} // namespace server
} // namespace osd

#endif // __STAMNOS_OSD_SERVER_OSD_LAYER_H
