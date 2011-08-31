#ifndef _ITREE_H_JKA111
#define _ITREE_H_JKA111

#include "chunkstore/chunkstore.h"

struct itree_entry {
	ChunkDescriptor* chunkdsc_;
	int              index_;    // entry index relative to chunkdsc->_chunk
};

int itree_create(void** itreep);
int itree_insert_bulk(void* itree, ChunkDescriptor* chunkdsc);
int itree_find_inode(void* itree, uint num, short type, int acl_id, struct dinode **dinp);

#endif
