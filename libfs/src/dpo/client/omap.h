/**
 * \file omap.h
 * 
 * \brief Object Map: maps object id (public object address) to 
 * process' private object header
 */

#ifndef __DPO_CLIENT_OBJECT_MAP_H_AKL189
#define __DPO_CLIENT_OBJECT_MAP_H_AKL189

#include "dpo/client/obj.h"
#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/debug.h"

typedef uint64_t ObjectId; 

namespace dpo {

namespace client {

class ObjectMap {
	typedef google::dense_hash_map<ObjectId, Object*> ObjectIdMap;
public:
	ObjectMap();
	int Init();
	int Lookup(ObjectId oid, Object** obj);
	int Insert(Object* obj);
	int Remove(ObjectId oid);
	int RemoveAll();

private:
	ObjectIdMap oid2obj_map_;
};


inline 
ObjectMap::ObjectMap()
{
	oid2obj_map_.set_empty_key(-1);
}


inline int 
ObjectMap::Lookup(ObjectId oid, Object** obj)
{
	google::dense_hash_map<ObjectId, Object*>::iterator it;

	it = oid2obj_map_.find(oid);

	if (it == oid2obj_map_.end()) {
		return -1;
	}
	*obj = it->second;
	return 0;
}


inline int 
ObjectMap::Insert(Object* obj)
{
	ObjectId                               oid;
	std::pair<ObjectIdMap::iterator, bool> pairret;

	oid = obj->oid();
	pairret = oid2obj_map_.insert(std::pair<ObjectId, Object*>(oid, obj));
	assert(pairret.second == true);

	return 0;
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

#endif // __DPO_CLIENT_INODE_MAP_H_AKL189
