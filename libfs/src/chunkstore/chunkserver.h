#ifndef _CHUNKSERVER_H_AGT127
#define _CHUNKSERVER_H_AGT127

#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <map>
#include <vector>
#include "lockserv/lockserv.h"
#include "common/pheap.h"
#include "common/vistaheap.h"
#include "common/list.h"
#include "chunkdsc.h"


#ifndef CHUNKSTOREROOT_DEFINED
#define CHUNKSTOREROOT_DEFINED
struct ChunkStoreRoot {
	int              _init;
	struct list_head _chunkdsc_list;
};
#endif



class ChunkServer {
public:
	void Init();
	int CreateChunk(int principal_id, size_t size, int type, ChunkDescriptor** chunkdscp);
	int DeleteChunk(int principal_id, ChunkDescriptor* chunkdsc);
	int AccessChunk(int principal_id, std::vector<ChunkDescriptor*> vchunkdsc, unsigned int prot_flags);
	int ReleaseChunk(int principal_id, std::vector<ChunkDescriptor*> vchunkdsc);
    int AccessAddr(int principal_id, void* addr);

	ChunkServer();
private:
	int CreateChunkVolatile(ChunkDescriptor *chunkdsc);

	id_t                                     _principal_id;
	PHeap*                                   _pheap; 
	PHeap*                                   _pagepheap; 
	ChunkStoreRoot*                          _pheap_root;
	pthread_mutex_t                          _mutex;
	std::map<unsigned long long, 
	         ChunkDescriptor*>               _addr2chunkdsc_map; 
	std::map<unsigned long long, 
	         ChunkDescriptorVolatile*>       _chunkdsc2chunkdscvol_map; 
};

#endif
