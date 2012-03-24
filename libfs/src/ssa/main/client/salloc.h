#ifndef __STAMNOS_SSA_CLIENT_STORAGE_ALLOCATOR_H
#define __STAMNOS_SSA_CLIENT_STORAGE_ALLOCATOR_H

#include <stdlib.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_set>
#include "bcs/bcs-opaque.h"
#include "ssa/main/common/obj.h"
#include "ssa/main/client/ssa-opaque.h"
#include "spa/pool/pool.h"


namespace ssa {
namespace client {

class StorageAllocator {
	typedef google::dense_hash_set< ::ssa::common::ObjectId, ::ssa::common::ObjectIdHashFcn > ObjectIdSet;
public:
	
	StorageAllocator(::client::Ipc* ipc)
		: ipc_(ipc)
	{ 
		for (int i=0; i<16; i++) {
			freeset_[i] = NULL;
		}
	}	
	int Load(StoragePool* pool);

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(SsaSession* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateExtent(SsaSession* session, size_t nbytes, int flags, void** ptr);

	int AllocateContainer(SsaSession* session, int type, ssa::common::ObjectId* oid);
	int AllocateContainerVector(SsaSession* session);

private:
	::client::Ipc*  ipc_;
	StoragePool*    pool_;
	ObjectIdSet*    freeset_[16]; // support 16 types: 0-15
};

} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_STORAGE_ALLOCATOR_H
