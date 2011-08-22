#ifndef _MPNODE_H_JAK129
#define _MPNODE_H_JAK129

#include <stdint.h>
#include "client/inode.h"
#include "client/sb.h"

namespace client {

// A pseudo inode representing a mount point


const int MAX_NUM_ENTRIES=2;



class MPInode: public Inode {
public:

	int Init(uint64_t ino) { return 0; }
	int Open(char* path, int flags) { return 0; }
	int Write(char* src, uint64_t off, uint64_t n) { return 0; }
	int Read(char* dst, uint64_t off, uint64_t n) { return 0; }
	int Lookup(char* name, Inode** inode);
	int LookupFast(char* name, Inode* inode) { return 0; }
	int Link(char* name, Inode* inode, bool overwrite) { return 0; }
	int Insert(char* name, Inode* inode);
	SuperBlock* GetSuperBlock() { return sb_; };
	void SetSuperBlock(SuperBlock* sb) { sb_ = sb; };

	struct Entry {
		char   name_[64];
		Inode* inode_;
	};

private:
	SuperBlock* sb_;
	Inode*      parent_;
	Entry       entries_[MAX_NUM_ENTRIES];
	int         entries_count_;
};

} // namespace client

#endif /* _MPNODE_H_JAK129 */
