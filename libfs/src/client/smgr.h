#ifndef _STORAGE_MANAGER_H_AKL783
#define _STORAGE_MANAGER_H_AKL783

#include <stdlib.h>
#include <typeinfo>
#include "rpc/rpc.h"
#include "chunkstore/chunkstore.h"

//FIXME: The Storage Manager should use the kernel storage API instead of the 
//chunkstore server when this facility becomes available.


namespace client {

class Session; // forward declaration

class StorageManager {
public:
	StorageManager(rpcc* c, unsigned int principal_id)
		: client_(c), 
		  principal_id_(principal_id),
		  chunk_store(c, principal_id)
	{ 
		chunk_store.Init();
	}		  
	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(Session* session, size_t nbytes, std::type_info const& typid, void** ptr);

private:
	rpcc*        client_;
	unsigned int principal_id_;
	ChunkStore   chunk_store;
};


} // namespace client

#endif /* _STORAGE_MANAGER_H_AKL783 */
