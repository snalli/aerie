#ifndef _CHUNKSTORE_H_AGT127
#define _CHUNKSTORE_H_AGT127

#include <sys/types.h>
#include "lockserv/lockserv.h"
#include "common/pheap.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"


class ChunkStore {
public:
	void Init();
	int CreateChunk(size_t size, ChunkDescriptor **chunkdscp);
	int DeleteChunk(ChunkDescriptor *chunkdsc);
	int AccessChunk(ChunkDescriptor *chunkdsc);

	ChunkStore(id_t);
private:
	id_t                  _principal_id;
	PHeap*                _pheap; 
	PHeap*                _pagepheap; 
	LockSpace*            _lockspace;
};


#endif
