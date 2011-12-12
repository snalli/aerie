#ifndef __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
#define __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "dpo/common/obj.h"
#include "dpo/client/proxy.h"
#include "dpo/client/omap.h"
#include "dpo/client/hlckmgr.h"

namespace dpo {

namespace client {

class ObjectManager; // forward reference

/**
 * \brief Type specific object manager
 */
class ObjectManagerOfType {
friend class ObjectManager;
public:
	virtual ObjectProxy* Create(ObjectId oid) = 0;
	//virtual void OnRelease();
	
private:
	ObjectMap oid2obj_map_;
};


class ObjectManager {
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	ObjectManager(dpo::cc::client::HLockManager* hlckmgr = NULL);
	int Register(ObjectType type_id, ObjectManagerOfType* mgr);
	int GetObject(ObjectId oid, dpo::common::ObjectProxyReference* obj_ref);
	int PutObject(dpo::common::ObjectProxyReference& obj_ref);
	int ReleaseObject(dpo::common::ObjectProxy* obj);
	//ObjectProxy* Object(ObjectId oid);
	//ObjectProxy* Object(ObjectId oid, ObjectProxy* obj);

private:
	pthread_mutex_t                mutex_;
	ObjectType2Manager             objtype2mgr_map_; 
	dpo::cc::client::HLockManager* hlckmgr_;
};


} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
