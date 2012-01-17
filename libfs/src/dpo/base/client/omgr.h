/**
 * \file omgr.h
 *
 * \brief Generic persistent object (container) manager
 * 
 */

#ifndef __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
#define __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "dpo/base/common/obj.h"
#include "dpo/base/client/proxy.h"
#include "dpo/base/client/omap.h"
#include "dpo/base/client/hlckmgr.h"
#include "client/session.h"

namespace client { class Session; } // forward declaration

namespace dpo {
namespace client {

class ObjectManager; // forward declaration

/**
 * \brief Abstract-base class of type specific persistent object manager
 */
class ObjectManagerOfType {
friend class ObjectManager;
public:
	virtual ObjectProxy* Create(::client::Session* session, ObjectId oid) = 0;
	virtual void OnRelease(::client::Session* session, ObjectId oid) = 0;

protected:	
	ObjectMap oid2obj_map_;
};


class ObjectManager: public dpo::cc::client::HLockUser {
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	ObjectManager(dpo::cc::client::LockManager* lckmgr, dpo::cc::client::HLockManager* hlckmgr);
	~ObjectManager();
	int RegisterType(ObjectType type_id, ObjectManagerOfType* mgr);
	int GetObject(ObjectId oid, dpo::common::ObjectProxyReference* obj_ref);
	int PutObject(dpo::common::ObjectProxyReference& obj_ref);
	int ReleaseObject(dpo::common::ObjectProxy* obj);
	void OnRelease(dpo::cc::client::HLock* hlock);
	void OnConvert(dpo::cc::client::HLock* hlock);
	int Revoke(dpo::cc::client::HLock* hlock) { }
	//ObjectProxy* Object(ObjectId oid);
	//ObjectProxy* Object(ObjectId oid, ObjectProxy* obj);
	int id() { return hlckmgr_->id(); }
private:
	int RegisterBaseTypes();
	
	pthread_mutex_t                mutex_;
	ObjectType2Manager             objtype2mgr_map_; 
	dpo::cc::client::LockManager*  lckmgr_;
	dpo::cc::client::HLockManager* hlckmgr_;
	::client::Session*             session_;
};


} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
