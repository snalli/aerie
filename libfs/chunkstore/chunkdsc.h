#ifndef _CHUNK_DESCRIPTOR_H_AJA123
#define _CHUNK_DESCRIPTOR_H_AJA123

enum {
	CHUNK_LOCK_READ = 0x0,
	CHUNK_LOCK_WRITE = 0x1,
	CHUNK_LOCK_NREADERS_MASK = 0xFFFFFFFE,
};

struct ChunkDescriptor {
	unsigned int  _owner_id; 
	void*         _chunk;    
	size_t        _size;      // chunk size
	//TODO: ACL (access control list)
};
typedef struct ChunkDescriptor ChunkDescriptor;

struct ChunkDescriptorVolatile {
	unsigned int  _locked;
	unsigned int  _proc_id; 
	unsigned int  _principal_id;
};
typedef struct ChunkDescriptorVolatile ChunkDescriptorVolatile;

#endif
