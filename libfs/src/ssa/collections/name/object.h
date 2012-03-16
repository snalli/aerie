//! \file
//! Definition of the name collection persistent object stored in SCM
//!


// TODO: move factory code into a separate class/file

#ifndef __STAMNOS_SSA_COMMON_NAME_COLLECTION_OBJECT_H
#define __STAMNOS_SSA_COMMON_NAME_COLLECTION_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "ssa/containers/name_collection/hashtable.h"
#include "ssa/main/common/obj.h"
#include "common/debug.h"
#include "common/util.h"

namespace ssa {
namespace containers {

class 
public ssa::client::rw::Object {

template<typename Session>
class NameCollection::ObjectProxy: 

public:
	static Object* Load(uint64_t ino) {
		return reinterpret_cast<NameCollection*>(ino);
	}
	
	Object()
		: ht_(NULL),
		  self_(ssa::common::ObjectId(0)),
		  parent_(ssa::common::ObjectId(0))
	{ 
		  nlink_ = 0;
	}

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		
		if (session->salloc_->Alloc(session, nbytes, typeid(NameCollection<Session>), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	int Find(Session* session, const char* name, ssa::common::Object oid);
	int Insert(Session* session, const char* name, ssa::common::ObjectId oid);
	int Erase(Session* session, const char* name);

//private:
	ObjectId            self_;    // entry '.'
	ObjectId            parent_;  // entry '..'
	HashTable<Session>* ht_;      // entries
};


template<typename Session>
int 
NameCollection<Session>::Lookup(Session* session, const char* name, uint64_t* ino)
{
	if (name[0] == '\0') {
		return -1;
	}	

	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			if (self_ == 0) {
				return -E_EXIST;
			}
			*ino = self_;
			return E_SUCCESS;
		case 2: // '..'
			if (self_ == 0) {
				return -E_EXIST;
			}
			*ino = parent_;
			return E_SUCCESS;
	}

	if (ht_) {
		return ht_->Search(session, name, strlen(name)+1, ino);
	}

	return -1;
}


template<typename Session>
int 
NameCollection<Session>::Link(Session* session, const char* name, uint64_t ino)
{
	uint64_t lino;

	if (name[0] == '\0') {
		return -1;
	}	
	
	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			self_ = ino;
			return 0;
		case 2: // '..'
			parent_ = ino;
			return 0;
	}
	
	if (!ht_) {
		ht_ = new(session) HashTable<Session>(); 
		if (!ht_) {
			return -1;
		}	
	}
	if (ht_->Search(session, name, strlen(name)+1, &lino)==0) {
		return -E_EXIST;
	}
	return ht_->Insert(session, name, strlen(name)+1, ino);
}


template<typename Session>
int 
NameCollection<Session>::Unlink(Session* session, const char* name)
{
	if (name[0] == '\0') {
		return -1;
	}	
	
	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			self_ = 0;
			return 0;
		case 2: // '..'
			parent_ = 0;
			return 0;
	}

	if (!ht_) {
		return -1;
	}
	return ht_->Delete(session, name, strlen(name)+1);
}

} // namespace common
} // namespace ssa

#endif // __STAMNOS_SSA_COMMON_NAME_COLLECTION_H
