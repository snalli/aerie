#ifndef _SNODE_H_JAK129
#define _SNODE_H_JAK129

#include <stdint.h>
#include "mfs/hashtable.h"

// Format of persistent inodes stored in SCM

class Snode {
public:
	static Snode* Load(uint64_t ino) {
		return reinterpret_cast<Snode*>(ino);
	}

	uint32_t   magic_;
	uint64_t   ino_;
	uint64_t   gen_;
	HashTable* ht_;
};



class DirSnode: public Snode {
public:
	static DirSnode* Load(uint64_t ino) {
		return reinterpret_cast<DirSnode*>(ino);
	}
	
	HashTable* ht_;
};



class FileSnode: public Snode {
public:
	static FileSnode* Load(uint64_t ino) {
		return reinterpret_cast<FileSnode*>(ino);
	}
	
};


#endif /* _INODE_H_JAK129 */
