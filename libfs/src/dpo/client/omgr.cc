/**
 * \file omgr.cc 
 *
 * \brief Object proxy manager
 */

#include "common/errno.h"
#include "dpo/common/omgr.cc"

namespace dpo {
namespace client {


ObjectManager::ObjectManager()
{
	pthread_mutex_lock(&mutex_);
	ObjectType2Manager.set_empty_key(NULL);
}


int
ObjectManager::Register(ObjectType* type_id, ObjectManagerOfType* mgr)
{
	int                          ret = E_SUCCESS;
	ObjectType2Manager::iterator itr; 

	pthread_mutex_lock(&mutex_);
    if ((itr = objtype2mgr_.find(type_id)) == objtype2mgr_.end()) {
		ret = -E_EXIST;
		goto done;
	}
	objtype2mgr_[type_id] = mgr;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


/**
 * \brief Initializes a reference to the object identified by oid
 */
int
ObjectManager::GetObject(ObjectId oid, ObjectProxyReference* obj_ref)
{
	pthread_mutex_lock(&mutex_);
	ObjectType type = oid->type();
	ObjectType2Manager::iterator itr;
	if ((itr = objtype2mgr_.find(type)) == objtype2mgr_.end()) {
		return -E_INVAL; // unknown type id 
	}

	pthread_mutex_unlock(&mutex_);
}


} // namespace client
} // namespace dpo
