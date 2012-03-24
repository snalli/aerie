//! \file
//! Definition of the superblock persistent object stored in SCM
//!


// TODO: move factory code into a separate class/file

#ifndef __STAMNOS_SSA_COMMON_SUPER_CONTAINER_OBJECT_H
#define __STAMNOS_SSA_COMMON_SUPER_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
#include "ssa/containers/typeid.h"
#include "ssa/main/common/obj.h"
#include "common/util.h"
#include "ssa/main/common/const.h"


namespace ssa {
namespace containers {
namespace common {

class SuperContainer {
public:
template<typename Session>
class Object: public ssa::cc::common::Object {
public:
	static Object* Make(Session* session) {
		void* ptr;
		
		if (session->salloc()->AllocateExtent(session, sizeof(Object), kMetadata, &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return new(ptr) Object();
	}

	static Object* Make(Session* session, volatile char* ptr) {
		return new((void*)ptr) Object();
	}

	static Object* Load(ssa::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}

	static Object* Load(void* ptr) {
		return reinterpret_cast<Object*>(ptr);
	}

	Object()
		: root_(ssa::common::ObjectId(0)),
		  freelist_(ssa::common::ObjectId(0))
	{ 
		set_type(T_SUPER_CONTAINER);
	}

	ssa::common::ObjectId root(Session* session);
	int set_root(Session* session, ssa::common::ObjectId oid);
	ssa::common::ObjectId freelist(Session* session);
	int set_freelist(Session* session, ssa::common::ObjectId oid);

	//int magic(Session* session);
	//int set_magic(Session* session, int magic);

private:
	int                    magic_;
	ssa::common::ObjectId  root_;  // 
	ssa::common::ObjectId  freelist_;  // the object containing the free block list
};

}; // class SuperContainer



template<typename Session>
ssa::common::ObjectId 
SuperContainer::Object<Session>::root(Session* session)
{
	return root_;
}


template<typename Session>
int 
SuperContainer::Object<Session>::set_root(Session* session, ssa::common::ObjectId oid)
{
	root_ = oid;
}


template<typename Session>
ssa::common::ObjectId 
SuperContainer::Object<Session>::freelist(Session* session)
{
	return freelist_;
}


template<typename Session>
int 
SuperContainer::Object<Session>::set_freelist(Session* session, ssa::common::ObjectId oid)
{
	freelist_ = oid;
}


} // namespace common
} // namespace containers
} // namespace ssa

#endif // __STAMNOS_SSA_COMMON_NAME_CONTAINER_H
