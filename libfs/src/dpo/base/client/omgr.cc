/**
 * \file omgr.cc 
 *
 * \brief Object proxy manager
 */

#include "common/errno.h"
#include "common/debug.h"
#include "common/list.h"
#include "dpo/base/client/lckmgr.h"
#include "dpo/base/client/hlckmgr.h"
#include "dpo/base/client/omgr.h"
#include "dpo/base/client/rwproxy.h"
#include "client/session.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"

//TODO: Fine-grain locking in GetObject/PutObject.


namespace dpo {
namespace client {


ObjectManager::ObjectManager(dpo::cc::client::LockManager* lckmgr, 
                             dpo::cc::client::HLockManager* hlckmgr)
	: lckmgr_(lckmgr),
	  hlckmgr_(hlckmgr)
{
	pthread_mutex_init(&mutex_, NULL);
	objtype2mgr_map_.set_empty_key(0);
	hlckmgr_->RegisterLockUser(this);
	//FIXME: the use of session is confusing as we don't use a complete session 
	// (for example this private session has a NULL pointer as a pointer to the generic object manager 
	session_ = new ::client::Session(lckmgr, hlckmgr, NULL, NULL);
	assert(RegisterBaseTypes() == E_SUCCESS);
}


ObjectManager::~ObjectManager()
{
//	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
//	        "[%d] Shutting down Object Manager\n", id());

	hlckmgr_->UnregisterLockUser();

}


int
ObjectManager::RegisterBaseTypes()
{
	int ret;

	// SuperContainer
	{
	dpo::client::rw::ObjectManager<dpo::containers::client::SuperContainer::Object, dpo::containers::client::SuperContainer::VersionManager>* mgr = new dpo::client::rw::ObjectManager<dpo::containers::client::SuperContainer::Object, dpo::containers::client::SuperContainer::VersionManager>;
    if ((ret = RegisterType(dpo::containers::T_SUPER_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	{
	// NameContainer
	dpo::client::rw::ObjectManager<dpo::containers::client::NameContainer::Object, dpo::containers::client::NameContainer::VersionManager>* mgr = new dpo::client::rw::ObjectManager<dpo::containers::client::NameContainer::Object, dpo::containers::client::NameContainer::VersionManager>;
    if ((ret = RegisterType(dpo::containers::T_NAME_CONTAINER, mgr)) < 0) {
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
	printf("type_id = %d\n", type_id);
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
ObjectManager::GetObjectInternal(ObjectId oid, 
                                 dpo::common::ObjectProxyReference** obj_refp, 
                                 bool use_exist_obj_ref)
{
	int                                ret = E_SUCCESS;
	ObjectManagerOfType*               mgr;
	ObjectProxy*                       objproxy;
	dpo::common::ObjectProxyReference* obj_ref;

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
		if ((objproxy = mgr->Load(session_, oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
		assert(mgr->oid2obj_map_.Insert(objproxy) == E_SUCCESS);
		// no need to grab the lock on obj after creation as it's not reachable 
		// before we release the lock manager's mutex lock
		obj_ref = new dpo::common::ObjectProxyReference();
		obj_ref->Set(objproxy, false);
	} else {
		// object proxy exists
		if (use_exist_obj_ref) {
			if ((obj_ref = objproxy->HeadReference()) == NULL) {
				obj_ref = new dpo::common::ObjectProxyReference();
				obj_ref->Set(objproxy, false);
			}
		} else {
			obj_ref = new dpo::common::ObjectProxyReference();
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
ObjectManager::FindObject(ObjectId oid, dpo::common::ObjectProxyReference** obj_ref) 
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());
	
	return GetObjectInternal(oid, obj_ref, true);
}


/**
 * \brief It returns a reference to the object proxy if the object identified by
 * oid exists. It increments the reference count.
 *
 */
int
ObjectManager::GetObject(ObjectId oid, dpo::common::ObjectProxyReference** obj_ref) 
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());
	
	return GetObjectInternal(oid, obj_ref, false);
}


int
ObjectManager::PutObject(dpo::common::ObjectProxyReference& obj_ref)
{
	int                  ret = E_SUCCESS;
	ObjectManagerOfType* mgr;
	ObjectProxy*         obj = obj_ref.obj();

	pthread_mutex_lock(&mutex_);
	obj_ref.Reset(true);
	pthread_mutex_unlock(&mutex_);
	return ret;
}


void 
ObjectManager::OnRelease(dpo::cc::client::HLock* hlock)
{
	ObjectId             oid(reinterpret_cast<uint64_t>(hlock->payload()));
	ObjectManagerOfType* mgr;

	pthread_mutex_lock(&mutex_);
	ObjectType type = oid.type();
	ObjectType2Manager::iterator itr;
	if ((itr = objtype2mgr_map_.find(type)) == objtype2mgr_map_.end()) {
		//ret = -E_INVAL; // unknown type id 
		goto done;
	}
	mgr = itr->second;
	
	mgr->OnRelease(session_, oid);
/*
	if ((ret = mgr->oid2obj_map_.Remove(oid) != E_SUCCESS) {
		ret = -E_EXIST; // does not exist
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_omgr), 
				"Object reference points to an unknown object oid\n");
	}
	obj_ref.Reset(true);
*/
done:
	pthread_mutex_unlock(&mutex_);
}


void 
ObjectManager::OnConvert(dpo::cc::client::HLock* hlock)
{
	assert(0);
}


int
ObjectManager::ReleaseObject(dpo::common::ObjectProxy* obj)
{
	int                  ret = E_SUCCESS;
	ObjectManagerOfType* mgr;
/*
	pthread_mutex_lock(&mutex_);
	ObjectType type = obj->oid.type();
	ObjectType2Manager::iterator itr;
	if ((itr = objtype2mgr_map_.find(type)) == objtype2mgr_map_.end()) {
		ret = -E_INVAL; // unknown type id 
		goto done;
	}
	mgr = itr->second;

	if ((ret = mgr->oid2obj_map_.Remove(obj->oid())) != E_SUCCESS) {
		ret = -E_EXIST; // does not exist
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_omgr), 
				"Object reference points to an unknown object oid\n");
	}
	obj_ref.Reset(true);
	pthread_mutex_unlock(&mutex_);
	return ret;
*/
}



} // namespace client
} // namespace dpo
