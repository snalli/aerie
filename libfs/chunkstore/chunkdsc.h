#ifndef _CHUNK_DESCRIPTOR_H_AJA123
#define _CHUNK_DESCRIPTOR_H_AJA123

#include "common/list.h"

enum {
	CHUNK_LOCK_READ = 0x0,
	CHUNK_LOCK_WRITE = 0x1,
	CHUNK_LOCK_NREADERS_MASK = 0xFFFFFFFE,
};


enum {
	CHUNK_TYPE_SUPERBLOCK = 1,
	CHUNK_TYPE_INODE = 2,
	CHUNK_TYPE_EXTENT = 3,
};


struct ChunkDescriptor {
	struct list_head _list;
	unsigned int     _owner_id; 
	void*            chunk_;    
	size_t           _size;      // chunk size
	int              _type;
	//TODO: ACL (access control list)
};
typedef struct ChunkDescriptor ChunkDescriptor;



// This struct is used to keep logically volatile state
// Alternatively, we could keep it in non-volatile memory and
// reset it after a restart.
// DESIGN_FIXME: IS the lock volatile or non-volatile? 
// What happens is the system crashes?
struct ChunkDescriptorVolatile {
	unsigned int  _locked;
	unsigned int  _proc_id; 
	unsigned int  _principal_id;
};
typedef struct ChunkDescriptorVolatile ChunkDescriptorVolatile;

#endif
