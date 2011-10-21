#include "client/smgr.h"
#include <stdlib.h>
#include <typeinfo>
#include "client/context.h"


namespace client {

// Storage manager
// It allocates storage for a new object:
// 1) from a local pool of objects (that match the ACL), or
// 2) from the kernel storage manager, or
// 3) by contacting the file system server
//
// the allocation function should take a transaction id as argument
// to ensure the atomicity of the operation
//



int 
StorageManager::Alloc(size_t nbytes, std::type_info const& typid, void** ptr)
{
	ChunkDescriptor* achunkdsc[16];

	chunk_store.CreateChunk(8192, CHUNK_TYPE_INODE, &achunkdsc[0]);
	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	*ptr = achunkdsc[0]->chunk_;
	//*ptr = malloc(nbytes);

	return 0;
}


int 
StorageManager::Alloc(ClientContext* ctx, size_t nbytes, std::type_info const& typid, void** ptr)
{
	ChunkDescriptor* achunkdsc[16];

	chunk_store.CreateChunk(8192, CHUNK_TYPE_INODE, &achunkdsc[0]);
	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	*ptr = achunkdsc[0]->chunk_;
	//*ptr = malloc(nbytes);

	return 0;
}

} // namespace client
