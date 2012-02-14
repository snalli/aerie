//! \file
//! Definition of the superblock persistent object stored in SCM
//!


// TODO: move factory code into a separate class/file

#ifndef __STAMNOS_DPO_COMMON_SUPER_CONTAINER_OBJECT_H
#define __STAMNOS_DPO_COMMON_SUPER_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "dpo/containers/typeid.h"
#include "dpo/main/common/obj.h"
#include "common/debug.h"
#include "common/util.h"


namespace dpo {
namespace containers {
namespace common {

class SuperContainer {
public:
template<typename Session>
class Object: public dpo::cc::common::Object {
public:
	static Object* Load(dpo::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}

	Object()
		: root_(dpo::common::ObjectId(0)),
		  free_(dpo::common::ObjectId(0))
	{ 
		set_type(T_SUPER_CONTAINER);
	}

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		
		if (session->smgr_->Alloc(session, nbytes, typeid(Object<Session>), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	dpo::common::ObjectId root(Session* session);
	int set_root(Session* session, dpo::common::ObjectId oid);

	//int magic(Session* session);
	//int set_magic(Session* session, int magic);

private:
	int                    magic_;
	dpo::common::ObjectId  root_;  // 
	dpo::common::ObjectId  free_;  // the object containing the free block list
};

}; // class SuperContainer



template<typename Session>
dpo::common::ObjectId 
SuperContainer::Object<Session>::root(Session* session)
{
	return root_;
}


template<typename Session>
int 
SuperContainer::Object<Session>::set_root(Session* session, dpo::common::ObjectId oid)
{
	root_ = oid;
}



} // namespace common
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_NAME_CONTAINER_H
