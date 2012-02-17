#ifndef __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H
#define __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H

#include <stdlib.h>
#include <typeinfo>
#include "chunkstore/chunkstore.h"
#include "ipc/main/client/ipc-opaque.h"

//FIXME: The Storage Manager should use the kernel storage API instead of the 
//chunkstore server when that facility becomes available.

namespace client {
class Session;  // forward declaration
} // namespace client


namespace dpo {
namespace client {

class StorageManager {
public:
	StorageManager(::client::Ipc* ipc)
		: ipc_(ipc),
		  chunk_store(ipc)
	{ 
		chunk_store.Init();
	}	

	int AllocateRaw(::client::Session* session, size_t size, void** ptr);
	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocExtent(::client::Session* session, size_t nbytes, void** ptr);

	int AllocateContainerVector(::client::Session* session);
private:
	::client::Ipc*  ipc_;
	ChunkStore      chunk_store;
};

} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H
