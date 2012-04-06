//! \file
//! Definition of the name collection persistent object stored in SCM
//!


#ifndef __STAMNOS_SSA_COMMON_NAME_CONTAINER_OBJECT_H
#define __STAMNOS_SSA_COMMON_NAME_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "ssa/containers/assoc/hashtable.h"
#include "ssa/containers/containers.h"
#include "ssa/main/client/salloc.h"
#include "ssa/main/common/obj.h"
#include "bcs/main/common/cdebug.h"
#include "common/util.h"

namespace ssa {
namespace containers {
namespace common {

class NameContainer {
public:
template<typename Session>
class Object: public ssa::cc::common::Object {
public:
	static Object* Make(Session* session) {
		ssa::common::ObjectId oid;
		
		if (session->salloc()->AllocateContainer(session, T_NAME_CONTAINER, &oid) < 0) {
			dbg_log(DBG_ERROR, "No storage available\n");
		}
		return Load(oid);
	}

	static Object* Make(Session* session, volatile char* ptr) {
		return new((void*)ptr) Object();
	}

	static Object* Load(ssa::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}
	
	Object()
		: self_(ssa::common::ObjectId(0))
	{ 
		set_type(T_NAME_CONTAINER);
	}

	int Find(Session* session, const char* name, ssa::common::ObjectId* oid);
	int Insert(Session* session, const char* name, ssa::common::ObjectId oid);
	int Erase(Session* session, const char* name);

	int Size(Session* sesion);

private:
	HashTable<Session>* ht() {
		return &ht_;
	}

	ssa::common::ObjectId  self_;    // entry '.'
	HashTable<Session>     ht_;      // entries
};

}; // class NameContainer


template<typename Session>
int 
NameContainer::Object<Session>::Find(Session* session, const char* name, ssa::common::ObjectId* oid)
{
	uint64_t u64;
	int      ret;

	if (name[0] == '\0') {
		return -1;
	}	

	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			if (self_ == ssa::common::ObjectId(0)) {
				return -E_EXIST;
			}
			*oid = self_;
			return E_SUCCESS;
		case 2: // '..'
			if (parent_ == ssa::common::ObjectId(0)) {
				return -E_EXIST;
			}
			*oid = parent_;
			return E_SUCCESS;
	}

	if ((ret = ht()->Search(session, name, strlen(name)+1, &u64)) < 0) {
		return ret;
	}
	*oid = ssa::common::ObjectId(u64);
	return E_SUCCESS;
}


template<typename Session>
int 
NameContainer::Object<Session>::Insert(Session* session, const char* name, ssa::common::ObjectId oid)
{
	uint64_t u64;

	dbg_log(DBG_DEBUG, "NameContainer %p, insert %s --> %p\n", this, name, oid.u64());

	if (name[0] == '\0') {
		return -1;
	}	
	
	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			self_ = oid;
			return E_SUCCESS;
		case 2: // '..'
			parent_ = oid;
			return E_SUCCESS;
	}
	
	if (ht()->Search(session, name, strlen(name)+1, &u64)==0) {
		return -E_EXIST;
	}
	return ht()->Insert(session, name, strlen(name)+1, oid.u64());
}


template<typename Session>
int 
NameContainer::Object<Session>::Erase(Session* session, const char* name)
{
	if (name[0] == '\0') {
		return -1;
	}	
	
	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			self_ = ssa::common::ObjectId(0);
			return E_SUCCESS;
		case 2: // '..'
			parent_ = ssa::common::ObjectId(0);
			return E_SUCCESS;
	}

	return ht()->Delete(session, name, strlen(name)+1);
}


template<typename Session>
int 
NameContainer::Object<Session>::Size(Session* session) 
{
	int size = 0;
	size += (self_ != ssa::common::ObjectId(0)) ? 1: 0;
	size += (parent_ != ssa::common::ObjectId(0)) ? 1: 0;
	size += ht_.Size(session); 
	return size;
}


} // namespace common
} // namespace containers
} // namespace ssa

#endif // __STAMNOS_SSA_COMMON_NAME_CONTAINER_H
