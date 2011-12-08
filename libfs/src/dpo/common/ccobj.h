#ifndef __STAMNOS_DPO_COMMON_CC_OBJECT_H
#define __STAMNOS_DPO_COMMON_CC_OBJECT_H

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"

// We support concurrency control along two (orthogonal) dimensions:
// 1) hierarchy: flat or hierarchical
//    - these two are mutually exclusive: an object cannot be controlled by
//      both a flat and hierarchical lock. 
// 2) synchronization type: locks or optimistic transactions
//    - currently we support read-only optimistic transactions
//    - these two modes are mutually exclusive

namespace dpo {

namespace cc {

namespace common {

typedef uint64_t Version;


// header of an object that is synchronized under distributed concurrency 
// control
class CCObject {
public:
	Object()
		: status_(0),
		  version_(0)
	{ }

	Version xVersion() { return version_; }
	Version xSetVersion(Version version) { version_ = version; }

protected:
	//! bit flags that indicate the state of the object
	//! - concurrency control mode
	uint64_t status_ __attribute__ ((aligned (8)));
	Version  version_ __attribute__ ((aligned (8)));
};


} // namespace common

} // namespace cc

} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_CC_OBJECT_H
