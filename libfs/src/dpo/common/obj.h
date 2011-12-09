/**
 * \file object.h
 *
 * \brief This is the object header of a public persistent object 
 */

#ifndef __STAMNOS_DPO_COMMON_PERSISTENT_OBJECT_H
#define __STAMNOS_DPO_COMMON_PERSISTENT_OBJECT_H

#include <stdint.h>
#include "common/types.h"

namespace dpo {

namespace common {

enum {
	OBJECT_NUMBER_LEN_LOG2 = 48
};

typedef uint16_t ObjectType;
typedef uint64_t ObjectId;

class Object {
public:
	ObjectId oid();
	ObjectType type();
	void set_type(ObjectType type);

private:
	ObjectType type_; //!< Magic number identifying object type
};

inline ObjectId
Object::oid()
{
	uint64_t oid = type_;
	oid = oid << OBJECT_NUMBER_LEN_LOG2;
	return oid + reinterpret_cast<uint64_t>(this);
}

inline ObjectType 
Object::type()
{
	return type_;
}

inline void 
Object::set_type(ObjectType type)
{
	type_ = type;
}


} // namespace common


namespace cc {

/**
 * We support concurrency control along two (orthogonal) dimensions:
 * 1) hierarchy: flat or hierarchical
 *    - these two are mutually exclusive: an object cannot be controlled by
 *      both a flat and hierarchical lock. 
 * 2) synchronization type: locks or optimistic transactions
 *    - currently we support read-only optimistic transactions
 *    - these two modes are mutually exclusive
 */

namespace common {

typedef uint64_t VersionNumber;


/**
 * Base object for any object that is synchronized under distributed 
 * concurrency control
 */
class Object: public dpo::common::Object {
public:
	Object()
		: status_(0),
		  version_(0)
	{ }

	VersionNumber xVersion() { return version_; }
	VersionNumber xSetVersion(VersionNumber version) { version_ = version; }

protected:
	//! bit flags that indicate the state of the object
	//! - concurrency control mode
	uint32_t      status_ __attribute__ ((aligned (4)));
	VersionNumber version_ __attribute__ ((aligned (8)));
};


} // namespace common

} // namespace cc



} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_OBJECT_H
