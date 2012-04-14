//! \file
//! Definition of the superblock persistent object stored in SCM
//!


// TODO: move factory code into a separate class/file

#ifndef __STAMNOS_OSD_COMMON_SUPER_CONTAINER_OBJECT_H
#define __STAMNOS_OSD_COMMON_SUPER_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
#include "osd/containers/containers.h"
#include "osd/main/common/obj.h"
#include "common/util.h"
#include "osd/main/common/const.h"


namespace osd {
namespace containers {
namespace common {

class SuperContainer {
public:
template<typename Session>
class Object: public osd::cc::common::Object {
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

	static Object* Load(osd::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}

	static Object* Load(void* ptr) {
		return reinterpret_cast<Object*>(ptr);
	}

	Object()
		: root_(osd::common::ObjectId(0)),
		  freelist_(osd::common::ObjectId(0))
	{ 
		set_type(T_SUPER_CONTAINER);
	}

	osd::common::ObjectId root(Session* session);
	void set_root(Session* session, osd::common::ObjectId oid);
	osd::common::ObjectId freelist(Session* session);
	void set_freelist(Session* session, osd::common::ObjectId oid);

	//int magic(Session* session);
	//int set_magic(Session* session, int magic);

private:
	int                    magic_;
	osd::common::ObjectId  root_;  // 
	osd::common::ObjectId  freelist_;  // the object containing the free block list
};

}; // class SuperContainer



template<typename Session>
osd::common::ObjectId 
SuperContainer::Object<Session>::root(Session* session)
{
	return root_;
}


template<typename Session>
void 
SuperContainer::Object<Session>::set_root(Session* session, osd::common::ObjectId oid)
{
	root_ = oid;
}


template<typename Session>
osd::common::ObjectId 
SuperContainer::Object<Session>::freelist(Session* session)
{
	return freelist_;
}


template<typename Session>
void
SuperContainer::Object<Session>::set_freelist(Session* session, osd::common::ObjectId oid)
{
	freelist_ = oid;
}


} // namespace common
} // namespace containers
} // namespace osd

#endif // __STAMNOS_OSD_COMMON_NAME_CONTAINER_H
