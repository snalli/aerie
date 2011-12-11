#ifndef __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
#define __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "dpo/common/obj.h"
#include "dpo/client/proxy.h"
#include "dpo/client/omap.h"

namespace dpo {

namespace client {

/**
 * \brief Type specific object manager
 */
class ObjectManagerOfType {
public:
	virtual void* Create();
	virtual void OnRelease();
private:
	ObjectMap oid2obj_map_;
};


class ObjectManager {
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	int Register(ObjectType* type, ObjectManagerOfType* mgr);
	//ObjectProxy* Object(ObjectId oid);
	//ObjectProxy* Object(ObjectId oid, ObjectProxy* obj);

private:
	pthread_mutex_t    mutex_;
	ObjectType2Manager objtype2mgr_; 
};


} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_MANAGER_H
