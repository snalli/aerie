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
	virtual ObjectProxy* Load(::client::Session* session, ObjectId oid) = 0;
	virtual void Publish(::client::Session* session, ObjectId oid) = 0;

protected:	
	ObjectMap oid2obj_map_;
};


class ObjectManager: public dpo::cc::client::HLockCallback, 
                     public dpo::cc::client::LockCallback 
{
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	ObjectManager(dpo::cc::client::LockManager* lckmgr, dpo::cc::client::HLockManager* hlckmgr);
	~ObjectManager();
	int RegisterType(ObjectType type_id, ObjectManagerOfType* mgr);
	int FindOrGetObject(::client::Session* session, ObjectId oid, dpo::common::ObjectProxyReference** obj_ref); 
	int FindObject(::client::Session* session, ObjectId oid, dpo::common::ObjectProxyReference** obj_ref); 
	int GetObject(::client::Session* session, ObjectId oid, dpo::common::ObjectProxyReference** obj_ref);
	int PutObject(::client::Session* session, dpo::common::ObjectProxyReference& obj_ref);
	int PublishObject(::client::Session* session, ObjectId oid);
	int id() { return hlckmgr_->id(); }
	
	// call-back methods
	void OnRelease(dpo::cc::client::HLock* hlock);
	void OnConvert(dpo::cc::client::HLock* hlock);
	void OnRelease(dpo::cc::client::Lock* lock);
	void OnConvert(dpo::cc::client::Lock* lock);

private:
	int GetObjectInternal(::client::Session* session, ObjectId oid, dpo::common::ObjectProxyReference** obj_ref, bool use_exist_obj_ref);
	int RegisterBaseTypes();
	
	pthread_mutex_t                mutex_;
	ObjectType2Manager             objtype2mgr_map_; 
	dpo::cc::client::LockManager*  lckmgr_;
	dpo::cc::client::HLockManager* hlckmgr_;
	::client::Session*             cb_session_; /**< the session used when calling call-back methods */
};


} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
