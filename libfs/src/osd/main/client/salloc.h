#ifndef __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H
#define __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H

#include <stdlib.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_set>
#include "bcs/bcs-opaque.h"
#include "osd/main/common/obj.h"
#include "osd/main/client/osd-opaque.h"
#include "spa/pool/pool.h"


namespace osd {
namespace client {

class StorageAllocator {
	typedef google::dense_hash_set< ::osd::common::ObjectId, ::osd::common::ObjectIdHashFcn > ObjectIdSet;
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
	int Alloc(OsdSession* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateExtent(OsdSession* session, size_t nbytes, int flags, void** ptr);

	int AllocateContainer(OsdSession* session, int type, osd::common::ObjectId* oid);
	int AllocateContainerVector(OsdSession* session);

private:
	::client::Ipc*  ipc_;
	StoragePool*    pool_;
	ObjectIdSet*    freeset_[16]; // support 16 types: 0-15
};

} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H
