/**
 * \file omap.h
 * 
 * \brief Object Map: maps object id (public object address) to 
 * process' private object proxy
 *
 */

#ifndef __STAMNOS_OSD_CLIENT_OBJECT_MAP_H
#define __STAMNOS_OSD_CLIENT_OBJECT_MAP_H

#include <stdint.h>
#include <google/dense_hash_map>
#include <google/sparse_hash_map>
#include "common/errno.h"
#include "osd/main/common/obj.h"
#include "osd/main/client/proxy.h"
#include <stdio.h>
#include <sys/types.h>
       #include <unistd.h>

#define N_MAP 8


namespace osd {

namespace client {


class __object_map {

	typedef google::dense_hash_map<ObjectId, ObjectProxy*, osd::common::ObjectIdHashFcn > ObjectIdMap;
public:
  typedef ObjectIdMap::iterator iterator;
        __object_map(){

		oid2obj_map_.set_empty_key(ObjectId(0));
        	oid2obj_map_.set_deleted_key(ObjectId(1));
        	pthread_spin_init(&(omap_lock), pshared);
	}

        int Insert(ObjectProxy** __proxy){
	
		ObjectId                               oid;
	        std::pair<ObjectIdMap::iterator, bool> pairret;
        	ObjectIdMap::iterator it;
	        ObjectProxy* proxy = *__proxy;

        	oid = proxy->oid();
	        pthread_spin_lock(&(omap_lock));

        	it = oid2obj_map_.find(oid);
	        if (it == oid2obj_map_.end())
        	{
                	pairret = oid2obj_map_.insert(std::pair<ObjectId, ObjectProxy*>(oid, proxy));
	                assert(pairret.second == true);
        	} else {
                	*__proxy = it->second;

	        }

        	pthread_spin_unlock(&(omap_lock));

	        return E_SUCCESS;

	}

        int Lookup(ObjectId oid, ObjectProxy** obj)
	{
	
	        ObjectIdMap::iterator it;

	        pthread_spin_lock(&(omap_lock));
	        it = oid2obj_map_.find(oid);

        	if (it == oid2obj_map_.end()) {
                	return -E_EXIST;
	        }
        	*obj = it->second;
        	pthread_spin_unlock(&(omap_lock));
	        return E_SUCCESS;

	}
        int Remove(ObjectId oid){}
        int RemoveAll(){}
        iterator begin() { return oid2obj_map_.begin(); }
        iterator end() { return oid2obj_map_.end(); }

private:
        ObjectIdMap oid2obj_map_;
	pthread_spinlock_t omap_lock; 
	int pshared;
};

class ObjectMap {
	typedef google::dense_hash_map<ObjectId, ObjectProxy*, osd::common::ObjectIdHashFcn > ObjectIdMap;
	// insight : key_type = ObjectID, data_type = ObjectProxy*, hash_fn = ObjectIDHashFcn
public:
	typedef ObjectIdMap::iterator iterator;
	ObjectMap();
	int Init();
	int Lookup(ObjectId oid, ObjectProxy** obj);
	int Insert(ObjectProxy** __proxy);
	int Remove(ObjectId oid);
	int RemoveAll();
	iterator begin() { return oid2obj_map_.begin(); }
	iterator end() { return oid2obj_map_.end(); }

private:
	ObjectIdMap oid2obj_map_;
	__object_map obj_map_arr[N_MAP];
	pthread_spinlock_t omap_lock; 
	int pshared;
	int i;
};


inline 
ObjectMap::ObjectMap()
{
	i = 0;
	oid2obj_map_.set_empty_key(ObjectId(0));
	oid2obj_map_.set_deleted_key(ObjectId(1));
	pthread_spin_init(&(omap_lock), pshared);
	
}


inline int 
ObjectMap::Lookup(ObjectId oid, ObjectProxy** obj)
{
	int index;
	index = oid.u64() % N_MAP;
	return obj_map_arr[index].Lookup(oid, obj);
/*	ObjectIdMap::iterator it;

	it = oid2obj_map_.find(oid);

	if (it == oid2obj_map_.end()) {
		return -E_EXIST;
	}
	*obj = it->second;
	return E_SUCCESS;
*/
}


inline int 
ObjectMap::Insert(ObjectProxy** __proxy)
{

	ObjectId                               oid;
	int index;
	ObjectProxy* proxy = *__proxy;

	oid = proxy->oid();
	index = oid.u64() % N_MAP;
	return obj_map_arr[index].Insert(__proxy);
/*        pthread_spin_lock(&(omap_lock));
	
	std::pair<ObjectIdMap::iterator, bool> pairret;
        ObjectIdMap::iterator it;
	it = oid2obj_map_.find(oid);
	if (it == oid2obj_map_.end())
	{
		pairret = oid2obj_map_.insert(std::pair<ObjectId, ObjectProxy*>(oid, proxy)); 
		assert(pairret.second == true);
	} else {
		*__proxy = it->second;
		
	}

        pthread_spin_unlock(&(omap_lock));

	return E_SUCCESS;
*/
}

inline int 
ObjectMap::Remove(ObjectId oid)
{
	int ret;
        printf("\nRemoving oid : %016llX", &oid);

	ret = oid2obj_map_.erase(oid);
	return ret;
}



} // namespace client

} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_OBJECT_MAP_H
