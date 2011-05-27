#ifndef _CHUNK_DESCRIPTOR_H_AJA123
#define _CHUNK_DESCRIPTOR_H_AJA123

struct ChunkDescriptor {
	unsigned int  _owner_id; 
	void*         _chunk;    
	size_t        _size;      // chunk size
	//TODO: ACL (access control list)
};
typedef struct ChunkDescriptor ChunkDescriptor;

#endif
