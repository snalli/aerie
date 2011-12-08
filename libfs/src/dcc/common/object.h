#ifndef __STAMNOS_DCC_COMMON_OBJECT_H
#define __STAMNOS_DCC_COMMON_OBJECT_H

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"

// Currently we support just read-only transactions
// For writes, one has to revert to using locks.


namespace dcc {

namespace common {

typedef uint64_t Version;


// header of an object that is synchronized under distributed concurrency 
// control
class Object {
public:
	Object()
		: version_(0)
	{ }

	Version xVersion() { return version_; }
	Version xSetVersion(Version version) { version_ = version; }

protected:
	Version version_ __attribute__ ((aligned (8)));
};



} // namespace common

} // namespace dcc

#endif // __STAMNOS_DCC_COMMON_OBJECT_H
