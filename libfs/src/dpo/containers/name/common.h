//! \file
//! Definition of the name collection persistent object stored in SCM
//!


#ifndef __STAMNOS_DPO_COMMON_NAME_CONTAINER_OBJECT_H
#define __STAMNOS_DPO_COMMON_NAME_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "dpo/containers/assoc/hashtable.h"
#include "dpo/containers/typeid.h"
#include "dpo/main/client/smgr.h"
#include "dpo/main/common/obj.h"
#include "common/debug.h"
#include "common/util.h"

namespace dpo {
namespace containers {
namespace common {

class NameContainer {
public:
template<typename Session>
class Object: public dpo::cc::common::Object {
public:
	static Object* Load(dpo::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}
	
	Object()
		: self_(dpo::common::ObjectId(0)),
		  parent_(dpo::common::ObjectId(0))
	{ 
		set_type(T_NAME_CONTAINER);
	}

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		
		if (session->smgr_->Alloc(session, nbytes, typeid(Object<Session>), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	int Find(Session* session, const char* name, dpo::common::ObjectId* oid);
	int Insert(Session* session, const char* name, dpo::common::ObjectId oid);
	int Erase(Session* session, const char* name);

	int Size(Session* sesion);

private:
	HashTable<Session>* ht() {
		return &ht_;
	}

	dpo::common::ObjectId  self_;    // entry '.'
	dpo::common::ObjectId  parent_;  // entry '..'
	HashTable<Session>     ht_;      // entries
};

}; // class NameContainer


template<typename Session>
int 
NameContainer::Object<Session>::Find(Session* session, const char* name, dpo::common::ObjectId* oid)
{
	uint64_t u64;
	int      ret;

	if (name[0] == '\0') {
		return -1;
	}	

	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			if (self_ == dpo::common::ObjectId(0)) {
				return -E_EXIST;
			}
			*oid = self_;
			return E_SUCCESS;
		case 2: // '..'
			if (parent_ == dpo::common::ObjectId(0)) {
				return -E_EXIST;
			}
			*oid = parent_;
			return E_SUCCESS;
	}

	if ((ret = ht()->Search(session, name, strlen(name)+1, &u64)) < 0) {
		return ret;
	}
	*oid = dpo::common::ObjectId(u64);
	return E_SUCCESS;
}


template<typename Session>
int 
NameContainer::Object<Session>::Insert(Session* session, const char* name, dpo::common::ObjectId oid)
{
	uint64_t u64;

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
			self_ = dpo::common::ObjectId(0);
			return E_SUCCESS;
		case 2: // '..'
			parent_ = dpo::common::ObjectId(0);
			return E_SUCCESS;
	}

	return ht()->Delete(session, name, strlen(name)+1);
}


template<typename Session>
int 
NameContainer::Object<Session>::Size(Session* session) 
{
	int size = 0;
	size += (self_ != dpo::common::ObjectId(0)) ? 1: 0;
	size += (parent_ != dpo::common::ObjectId(0)) ? 1: 0;
	size += ht_.Size(session); 
	return size;
}


} // namespace common
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_NAME_CONTAINER_H
