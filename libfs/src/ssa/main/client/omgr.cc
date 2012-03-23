/**
 * \file omgr.cc 
 *
 * \brief Object proxy manager
 */

#include "common/errno.h"
#include "bcs/bcs.h"
#include "ssa/main/client/stsystem.h"
#include "ssa/main/client/lckmgr.h"
#include "ssa/main/client/hlckmgr.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/main/client/session.h"
#include "ssa/containers/super/container.h"
#include "ssa/containers/name/container.h"
#include "ssa/containers/byte/container.h"

//TODO: Fine-grain locking in GetObject/PutObject.


namespace ssa {
namespace client {


/**
 * The storage system must have properly initialized lock managers
 */
ObjectManager::ObjectManager(ssa::client::StorageSystem* stsystem)
	: stsystem_(stsystem)
{
	pthread_mutex_init(&mutex_, NULL);
	objtype2mgr_map_.set_empty_key(0);
	stsystem_->hlckmgr()->RegisterLockCallback(this);
	stsystem_->lckmgr()->RegisterLockCallback(::ssa::cc::client::Lock::TypeId, this);
	id_ = stsystem_->ipc()->id();
	cb_session_ = new ::client::Session(stsystem_);
	assert(RegisterBaseTypes() == E_SUCCESS);
}


ObjectManager::~ObjectManager()
{
//	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
//	        "[%d] Shutting down Object Manager\n", id());

	stsystem_->hlckmgr()->UnregisterLockCallback();
	stsystem_->lckmgr()->UnregisterLockCallback(::ssa::cc::client::Lock::TypeId);

}


int
ObjectManager::RegisterBaseTypes()
{
	int ret;

	// SuperContainer
	{
	ssa::client::rw::ObjectManager<ssa::containers::client::SuperContainer::Object, ssa::containers::client::SuperContainer::VersionManager>* mgr = new ssa::client::rw::ObjectManager<ssa::containers::client::SuperContainer::Object, ssa::containers::client::SuperContainer::VersionManager>;
    if ((ret = RegisterType(ssa::containers::T_SUPER_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	{
	// NameContainer
	ssa::client::rw::ObjectManager<ssa::containers::client::NameContainer::Object, ssa::containers::client::NameContainer::VersionManager>* mgr = new ssa::client::rw::ObjectManager<ssa::containers::client::NameContainer::Object, ssa::containers::client::NameContainer::VersionManager>;
    if ((ret = RegisterType(ssa::containers::T_NAME_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	{
	// ByteContainer
	ssa::client::rw::ObjectManager<ssa::containers::client::ByteContainer::Object, ssa::containers::client::ByteContainer::VersionManager>* mgr = new ssa::client::rw::ObjectManager<ssa::containers::client::ByteContainer::Object, ssa::containers::client::ByteContainer::VersionManager>;
    if ((ret = RegisterType(ssa::containers::T_BYTE_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	return E_SUCCESS;
}


int
ObjectManager::RegisterType(ObjectType type_id, ObjectManagerOfType* mgr)
{
	int                          ret = E_SUCCESS;
	ObjectType2Manager::iterator itr; 

	pthread_mutex_lock(&mutex_);
    if ((itr = objtype2mgr_map_.find(type_id)) != objtype2mgr_map_.end()) {
		ret = -E_EXIST;
		printf("EXISTS\n");
		goto done;
	}
	objtype2mgr_map_[type_id] = mgr;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


/**
 * \brief Initializes a reference to the object identified by oid
 *
 * If use_exist_obj_ref is set then it sets obj_ref to point to an existing
 * reference that points to the object. This is useful for embedded references
 * Otherwise it initializes the obj_ref to point to the object. 
 */
int
ObjectManager::GetObjectInternal(::client::Session* session,
                                 ObjectId oid, 
                                 ssa::common::ObjectProxyReference** obj_refp, 
                                 bool use_exist_obj_ref)
{
	int                                ret = E_SUCCESS;
	ObjectManagerOfType*               mgr;
	ObjectProxy*                       objproxy;
	ssa::common::ObjectProxyReference* obj_ref;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());

	pthread_mutex_lock(&mutex_);
	ObjectType type = oid.type();
	ObjectType2Manager::iterator itr;
	if ((itr = objtype2mgr_map_.find(type)) == objtype2mgr_map_.end()) {
		ret = -E_INVAL; // unknown type id 
		goto done;
	}
	mgr = itr->second;
	if ((ret = mgr->oid2obj_map_.Lookup(oid, &objproxy)) != E_SUCCESS) {
		// create the object proxy
		if ((objproxy = mgr->Load(cb_session_, oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
		assert(mgr->oid2obj_map_.Insert(objproxy) == E_SUCCESS);
		// no need to grab the lock on obj after creation as it's not reachable 
		// before we release the lock manager's mutex lock
		obj_ref = new ssa::common::ObjectProxyReference();
		obj_ref->Set(objproxy, false);
	} else {
		// object proxy exists
		if (use_exist_obj_ref) {
			if ((obj_ref = objproxy->HeadReference()) == NULL) {
				obj_ref = new ssa::common::ObjectProxyReference();
				obj_ref->Set(objproxy, false);
			}
		} else {
			obj_ref = new ssa::common::ObjectProxyReference();
			obj_ref->Set(objproxy, true);
		}
	}
	ret = E_SUCCESS;
	*obj_refp = obj_ref;
done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


/**
 * \brief It returns a reference to the object proxy if the object identified 
 * by oid exists.
 * If a reference already exists it returns the existing reference without
 * incrementing the refcount.
 * If a reference does not exist it creates and returns a new reference 
 * (with refcount=1)
 *
 */
int
ObjectManager::FindObject(::client::Session* session, 
                          ObjectId oid, 
                          ssa::common::ObjectProxyReference** obj_ref) 
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());
	
	return GetObjectInternal(session, oid, obj_ref, true);
}


/**
 * \brief It returns a reference to the object proxy if the object identified by
 * oid exists. It increments the reference count.
 *
 */
int
ObjectManager::GetObject(::client::Session* session, 
                         ObjectId oid, 
                         ssa::common::ObjectProxyReference** obj_ref) 
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());
	
	return GetObjectInternal(session, oid, obj_ref, false);
}


int
ObjectManager::PutObject(::client::Session* session, 
                         ssa::common::ObjectProxyReference& obj_ref)
{
	int                  ret = E_SUCCESS;
	ObjectManagerOfType* mgr;

	pthread_mutex_lock(&mutex_);
	obj_ref.Reset(true);
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int
ObjectManager::CloseObject(::client::Session* session, ObjectId oid, bool update)
{
	int                  ret = E_SUCCESS;
	ObjectManagerOfType* mgr;

	pthread_mutex_lock(&mutex_);
	ObjectType type = oid.type();
	ObjectType2Manager::iterator itr;
	if ((itr = objtype2mgr_map_.find(type)) == objtype2mgr_map_.end()) {
		goto done;
	}
	mgr = itr->second;
	
	mgr->Close(session, oid, update);

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


/**
 * \brief Call-back made by the lock manager when a hierarchical lock is released 
 */
void 
ObjectManager::OnRelease(ssa::cc::client::HLock* hlock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(hlock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


/**
 * \brief Call-back made by the lock manager when a hierarchical lock is down-graded 
 */
void 
ObjectManager::OnConvert(ssa::cc::client::HLock* hlock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(hlock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


/**
 * \brief Call-back made by the lock manager when a flat lock is released 
 */
void 
ObjectManager::OnRelease(ssa::cc::client::Lock* lock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(lock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


/**
 * \brief Call-back made by the lock manager when a flat lock is down-graded 
 */
void 
ObjectManager::OnConvert(ssa::cc::client::Lock* lock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(lock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


void
ObjectManager::CloseAllObjects(::client::Session* session, bool update)
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), "[%d] Close all objects\n", id());
	
	ObjectManagerOfType*         mgr;
	ObjectType2Manager::iterator itr;
	
	pthread_mutex_lock(&mutex_);
	
	// flush the log of updates to the server
	if (update) {


	}

	// now close all the objects 
	for (itr = objtype2mgr_map_.begin(); itr != objtype2mgr_map_.end(); itr++) {
		mgr = itr->second;
		mgr->CloseAll(session, false);
	}

	pthread_mutex_unlock(&mutex_);
}


void
ObjectManager::PreDowngrade()
{
	CloseAllObjects(cb_session_, true);
}


} // namespace client
} // namespace ssa
