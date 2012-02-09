#ifndef __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H
#define __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H

#include <stdlib.h>
#include <typeinfo>
#include "rpc/rpc.h"
#include "chunkstore/chunkstore.h"

//FIXME: The Storage Manager should use the kernel storage API instead of the 
//chunkstore server when that facility becomes available.

namespace client {
class Session; // forward declaration
} // namespace client


namespace dpo {
namespace client {

class StorageManager {
public:
	StorageManager(rpcc* cl2srv, unsigned int principal_id)
		: cl2srv_(cl2srv), 
		  principal_id_(principal_id),
		  chunk_store(cl2srv, principal_id)
	{ 
		chunk_store.Init();
	}	

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocExtent(::client::Session* session, size_t nbytes, void** ptr);

	int AllocateContainerVector(::client::Session* session);
private:
	rpcc*        cl2srv_;
	unsigned int principal_id_;
	ChunkStore   chunk_store;
};

} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_STORAGE_MANAGER_H
