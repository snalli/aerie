/**
 * \file omgr.cc 
 *
 * \brief Object proxy manager
 */

#include "common/errno.h"
#include "common/debug.h"
#include "common/list.h"
#include "dpo/client/hlckmgr.h"
#include "dpo/client/omgr.h"

namespace dpo {
namespace client {

//TODO: Fine-grain locking in GetObject/PutObject.

ObjectManager::ObjectManager(dpo::cc::client::HLockManager* hlckmgr)
	: hlckmgr_(hlckmgr)
{
	pthread_mutex_init(&mutex_, NULL);
	objtype2mgr_map_.set_empty_key(0);
}


int
ObjectManager::Register(ObjectType type_id, ObjectManagerOfType* mgr)
{
	int                          ret = E_SUCCESS;
	ObjectType2Manager::iterator itr; 

	pthread_mutex_lock(&mutex_);
    if ((itr = objtype2mgr_map_.find(type_id)) != objtype2mgr_map_.end()) {
		ret = -E_EXIST;
		goto done;
	}
	objtype2mgr_map_[type_id] = mgr;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


/**
 * \brief Initializes a reference to the object identified by oid
 */
int
ObjectManager::GetObject(ObjectId oid, dpo::common::ObjectProxyReference* obj_ref)
{
	int                  ret = E_SUCCESS;
	ObjectManagerOfType* mgr;
	ObjectProxy*         obj;

	pthread_mutex_lock(&mutex_);
	ObjectType type = oid.type();
	ObjectType2Manager::iterator itr;
	if ((itr = objtype2mgr_map_.find(type)) == objtype2mgr_map_.end()) {
		ret = -E_INVAL; // unknown type id 
		goto done;
	}
	mgr = itr->second;
	if ((ret = mgr->oid2obj_map_.Lookup(oid, &obj)) != E_SUCCESS) {
		// create the object
		if ((obj = mgr->Create(oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
		assert(mgr->oid2obj_map_.Insert(obj) == E_SUCCESS);
		// no need to grab the lock on obj after creation as it's not reachable 
		// before we release the lock manager's mutex lock
		obj_ref->Set(obj, false);
	} else {
		// object exists
		obj_ref->Set(obj, true);
	}
	ret = E_SUCCESS;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
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
