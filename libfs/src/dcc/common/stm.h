#ifndef _TRANSACTION_STAMNOS_H_BMA567
#define _TRANSACTION_STAMNOS_H_BMA567

#include <setjmp.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"

// Currently we support just read-only transactions
// For writes, one has to revert to using locks.


namespace dcc {

namespace stm {

typedef uint64_t Version;


// header of a transactional object
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



} // namespace stm

} // namespace dcc

#endif // _TRANSACTION_STAMNOS_H_BMA567
