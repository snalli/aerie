/**
 * \file omgr.h
 *
 * \brief Generic persistent object (container) manager
 * 
 */

#ifndef __STAMNOS_SSA_CLIENT_OBJECT_MANAGER_H
#define __STAMNOS_SSA_CLIENT_OBJECT_MANAGER_H

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "ssa/main/common/obj.h"
#include "ssa/main/client/proxy.h"
#include "ssa/main/client/omap.h"
#include "ssa/main/client/hlckmgr.h"
#include "client/session.h"

namespace client { class Session; } // forward declaration

namespace ssa {
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


class ObjectManager: public ssa::cc::client::HLockCallback, 
                     public ssa::cc::client::LockCallback 
{
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	ObjectManager(ssa::cc::client::LockManager* lckmgr, ssa::cc::client::HLockManager* hlckmgr);
	~ObjectManager();
	int RegisterType(ObjectType type_id, ObjectManagerOfType* mgr);
	int FindOrGetObject(::client::Session* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref); 
	int FindObject(::client::Session* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref); 
	int GetObject(::client::Session* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref);
	int PutObject(::client::Session* session, ssa::common::ObjectProxyReference& obj_ref);
	int PublishObject(::client::Session* session, ObjectId oid);
	int id() { return hlckmgr_->id(); }
	
	// call-back methods
	void OnRelease(ssa::cc::client::HLock* hlock);
	void OnConvert(ssa::cc::client::HLock* hlock);
	void OnRelease(ssa::cc::client::Lock* lock);
	void OnConvert(ssa::cc::client::Lock* lock);

private:
	int GetObjectInternal(::client::Session* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref, bool use_exist_obj_ref);
	int RegisterBaseTypes();
	
	pthread_mutex_t                mutex_;
	ObjectType2Manager             objtype2mgr_map_; 
	ssa::cc::client::LockManager*  lckmgr_;
	ssa::cc::client::HLockManager* hlckmgr_;
	::client::Session*             cb_session_; /**< the session used when calling call-back methods */
};


} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_OBJECT_MANAGER_H
