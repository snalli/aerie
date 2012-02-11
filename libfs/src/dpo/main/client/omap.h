/**
 * \file omap.h
 * 
 * \brief Object Map: maps object id (public object address) to 
 * process' private object proxy
 *
 */

#ifndef __STAMNOS_DPO_CLIENT_OBJECT_MAP_H
#define __STAMNOS_DPO_CLIENT_OBJECT_MAP_H

#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/debug.h"
#include "common/errno.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/client/proxy.h"

namespace dpo {

namespace client {

class ObjectMap {
	typedef google::dense_hash_map<ObjectId, ObjectProxy*, dpo::common::ObjectIdHashFcn > ObjectIdMap;
public:
	ObjectMap();
	int Init();
	int Lookup(ObjectId oid, ObjectProxy** obj);
	int Insert(ObjectProxy* obj);
	int Remove(ObjectId oid);
	int RemoveAll();

private:
	ObjectIdMap oid2obj_map_;
};


inline 
ObjectMap::ObjectMap()
{
	oid2obj_map_.set_empty_key(ObjectId(0));
}


inline int 
ObjectMap::Lookup(ObjectId oid, ObjectProxy** obj)
{
	ObjectIdMap::iterator it;

	it = oid2obj_map_.find(oid);

	if (it == oid2obj_map_.end()) {
		return -E_EXIST;
	}
	*obj = it->second;
	return E_SUCCESS;
}


inline int 
ObjectMap::Insert(ObjectProxy* obj)
{
	ObjectId                               oid;
	std::pair<ObjectIdMap::iterator, bool> pairret;

	oid = obj->oid();
	pairret = oid2obj_map_.insert(std::pair<ObjectId, ObjectProxy*>(oid, obj));
	assert(pairret.second == true);

	return E_SUCCESS;
}

inline int 
ObjectMap::Remove(ObjectId oid)
{
	int ret;

	ret = oid2obj_map_.erase(oid);
	return ret;
}

} // namespace client

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_MAP_H
