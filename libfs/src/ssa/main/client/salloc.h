#ifndef __STAMNOS_SSA_CLIENT_STORAGE_ALLOCATOR_H
#define __STAMNOS_SSA_CLIENT_STORAGE_ALLOCATOR_H

#include <stdlib.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_set>
#include "ipc/main/client/ipc-opaque.h"
#include "ssa/main/common/obj.h"
#include "spa/pool/pool.h"


namespace client {
class Session;  // forward declaration
} // namespace client


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
	int Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateExtent(::client::Session* session, size_t nbytes, void** ptr);

	int AllocateContainer(::client::Session* session, int type, ssa::common::ObjectId* oid);
	int AllocateContainerVector(::client::Session* session);

private:
	::client::Ipc*  ipc_;
	StoragePool*    pool_;
	ObjectIdSet*    freeset_[16]; // support 16 types: 0-15
};

} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_STORAGE_ALLOCATOR_H
