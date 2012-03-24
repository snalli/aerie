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
#include "ssa/main/client/ssa-opaque.h"
#include "ssa/main/client/session.h"


namespace ssa {
namespace client {

class ObjectManager; // forward declaration

/**
 * \brief Abstract-base class of type specific persistent object manager
 */
class ObjectManagerOfType {
friend class ObjectManager;
public:
	virtual ObjectProxy* Load(SsaSession* session, ObjectId oid) = 0;
	virtual void Close(SsaSession* session, ObjectId oid, bool update) = 0;
	virtual void CloseAll(SsaSession* session, bool update) = 0;

protected:	
	ObjectMap oid2obj_map_;
};


class ObjectManager: public ssa::cc::client::HLockCallback, 
                     public ssa::cc::client::LockCallback 
{
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	ObjectManager(ssa::client::StorageSystem* stsystem);
	~ObjectManager();
	int RegisterType(ObjectType type_id, ObjectManagerOfType* mgr);
	int FindOrGetObject(SsaSession* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref); 
	int FindObject(SsaSession* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref); 
	int GetObject(SsaSession* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref);
	int PutObject(SsaSession* session, ssa::common::ObjectProxyReference& obj_ref);
	int CloseObject(SsaSession* session, ObjectId oid, bool update);
	int id() { return id_; }
	void CloseAllObjects(SsaSession* session, bool update);

	// call-back methods
	void PreDowngrade();
	void OnRelease(ssa::cc::client::HLock* hlock);
	void OnConvert(ssa::cc::client::HLock* hlock);
	void OnRelease(ssa::cc::client::Lock* lock);
	void OnConvert(ssa::cc::client::Lock* lock);

private:
	int GetObjectInternal(SsaSession* session, ObjectId oid, ssa::common::ObjectProxyReference** obj_ref, bool use_exist_obj_ref);
	int RegisterBaseTypes();
	
	int                          id_;
	pthread_mutex_t              mutex_;
	ObjectType2Manager           objtype2mgr_map_; 
	ssa::client::StorageSystem*  stsystem_;
	SsaSession*                  cb_session_; /**< the session used when calling call-back methods */
};


} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_OBJECT_MANAGER_H
