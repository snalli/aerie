#ifndef _CHUNKSTORE_H_AGT127
#define _CHUNKSTORE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"
#include "lockserv/lockserv.h"
#include "common/pheap.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"

namespace client {
class Ipc;
} // namespace client

#ifndef CHUNKSTOREROOT_DEFINED
#define CHUNKSTOREROOT_DEFINED
struct ChunkStoreRoot {
	int              _init;
	struct list_head _chunkdsc_list;
};
#endif

class ChunkStore {
public:
	void Init();
	int CreateChunk(size_t size, int type, ChunkDescriptor **chunkdscp);
	int DeleteChunk(ChunkDescriptor *chunkdsc);
	int AccessChunkList(ChunkDescriptor* chunkdsc[], size_t nchunkdsc, int prot_flags);
	int AccessChunk(ChunkDescriptor* chunkdsc, int prot_flags);
	int ReleaseChunkList(ChunkDescriptor* chunkdsc[], size_t nchunkdsc);
	int ReleaseChunk(ChunkDescriptor* chunkdsc);
	int AccessAddr(void* addr);

	ChunkStoreRoot* get_root() {
		return _pheap_root;
	}

	ChunkStore(::client::Ipc* ipc_layer);
private:
	PHeap*           _pheap; 
	PHeap*           _pagepheap; 
	ChunkStoreRoot*  _pheap_root;
	::client::Ipc*   ipc_;
};


#endif
