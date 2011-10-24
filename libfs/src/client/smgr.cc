#include "client/smgr.h"
#include <stdlib.h>
#include <typeinfo>
#include "client/session.h"


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
	assert(0);
}


int 
StorageManager::Alloc(Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	chunk_store.CreateChunk(roundup_bytes, CHUNK_TYPE_INODE, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
}

} // namespace client
