//! \file
//! Definition of the unordered map associative collection persistent object stored in SCM
//!


#ifndef __STAMNOS_OSD_COMMON_MAP_CONTAINER_OBJECT_H
#define __STAMNOS_OSD_COMMON_MAP_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "osd/containers/map/hashtable.h"
#include "osd/containers/containers.h"
#include "osd/main/client/salloc.h"
#include "osd/main/common/obj.h"
#include "bcs/main/common/cdebug.h"
#include "common/util.h"


namespace osd {
namespace containers {
namespace common {

class MapContainer {
public:
template<typename Session>
class Object: public osd::cc::common::Object {
public:
	static Object* Make(Session* session, osd::common::AclIdentifier acl_id = 0) {
		osd::common::ObjectId oid;
		
		if (session->salloc()->AllocateContainer(session, acl_id, T_MAP_CONTAINER, &oid) < 0) {
			dbg_log(DBG_ERROR, "No storage available\n");
		}
		return Load(oid);
	}

	static Object* Make(Session* session, volatile char* ptr) {
		return new((void*)ptr) Object();
	}

	static Object* Load(osd::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}
	
	Object()
		: self_(osd::common::ObjectId(0))
	{ 
		set_type(T_MAP_CONTAINER);
	}

	int Find(Session* session, const char* key, osd::common::ObjectId* oid);
	int Insert(Session* session, const char* key, osd::common::ObjectId oid);
	int Erase(Session* session, const char* key);

	int Size(Session* sesion);

private:
	HashTable<Session>* ht() {
		return &ht_;
	}

	HashTable<Session>     ht_;      // entries
};

}; // class MapContainer


template<typename Session>
int 
MapContainer::Object<Session>::Find(Session* session, const char* name, osd::common::ObjectId* oid)
{
	uint64_t u64;
	int      ret;

	if (name[0] == '\0') {
		return -1;
	}	
	if ((ret = ht()->Search(session, name, strlen(name)+1, &u64)) < 0) {
		return ret;
	}
	*oid = osd::common::ObjectId(u64);
	return E_SUCCESS;
}


template<typename Session>
int 
MapContainer::Object<Session>::Insert(Session* session, const char* name, osd::common::ObjectId oid)
{
	uint64_t u64;

	dbg_log(DBG_DEBUG, "MapContainer %p, insert %s --> %p\n", this, name, oid.u64());

	if (name[0] == '\0') {
		return -1;
	}	
	if (ht()->Search(session, name, strlen(name)+1, &u64)==0) {
		return -E_EXIST;
	}
	return ht()->Insert(session, name, strlen(name)+1, oid.u64());
}


template<typename Session>
int 
MapContainer::Object<Session>::Erase(Session* session, const char* name)
{
	if (name[0] == '\0') {
		return -1;
	}	
	return ht()->Delete(session, name, strlen(name)+1);
}


template<typename Session>
int 
MapContainer::Object<Session>::Size(Session* session) 
{
	int size = 0;
	size += (self_ != osd::common::ObjectId(0)) ? 1: 0;
	size += (parent_ != osd::common::ObjectId(0)) ? 1: 0;
	size += ht_.Size(session); 
	return size;
}


} // namespace common
} // namespace containers
} // namespace osd

#endif // __STAMNOS_OSD_COMMON_MAP_CONTAINER_H
