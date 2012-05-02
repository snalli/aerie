//! \file
//! Definition of the name collection persistent object stored in SCM
//!

// TODO: any allocations and assignments done by Mapslot must be journalled

#ifndef __STAMNOS_OSD_COMMON_NEEDLE_CONTAINER_OBJECT_H
#define __STAMNOS_OSD_COMMON_NEEDLE_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "osd/containers/containers.h"
#include "osd/main/common/obj.h"
#include "osd/main/common/const.h"
#include "scm/const.h"
#include "bcs/main/common/cdebug.h"
#include "common/util.h"

namespace osd {
namespace containers {
namespace common {

class NeedleContainer {
public:


template<typename Session>
class Object: public osd::cc::common::Object {
	enum  { 
		kNeedleSize = 32*1024 // be careful to not overflow (sizeof(enum) = sizeof(int))
	};
public:
	static Object* Make(Session* session, osd::common::AclIdentifier acl_id = 0) {
		osd::common::ObjectId oid;
		
		if (session->salloc()->AllocateContainer(session, acl_id, T_NEEDLE_CONTAINER, &oid) < 0) {
			dbg_log(DBG_ERROR, "No storage available\n");
		}
		return Load(oid);
	}

	static Object* Make(Session* session, volatile char* ptr) {
		return new((void*) ptr) Object();
	}

	static Object* Load(osd::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}
	
	Object();

	int Write(Session* session, const char* src, uint64_t off, uint64_t n);
	int Read(Session* session, char* dst, uint64_t off, uint64_t n);

	uint64_t Size() { return size_; }

	uint64_t size_;
	char     byte_[kNeedleSize - sizeof(uint64_t)];

}; // Object

};

template<typename Session>
NeedleContainer::Object<Session>::Object()
	: size_(0)
{
	set_type(T_NEEDLE_CONTAINER);
}


template<typename Session>
int 
NeedleContainer::Object<Session>::Write(Session* session, const char* src, uint64_t off, uint64_t n)
{
	memcpy(byte_, src, n);
	size_ = n;
	return n;
}


template<typename Session>
int 
NeedleContainer::Object<Session>::Read(Session* session, char* dst, uint64_t off, uint64_t n)
{
	memcpy(dst, byte_, n);
	return n;
}


} // namespace common
} // namespace containers
} // namespace osd

#endif // __STAMNOS_OSD_COMMON_NEEDLE_CONTAINER_H
