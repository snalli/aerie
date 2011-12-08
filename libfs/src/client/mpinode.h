#ifndef __STAMNOS_FS_CLIENT_MPINODE_H
#define __STAMNOS_FS_CLIENT_MPINODE_H

#include <stdint.h>
#include "client/inode.h"
#include "client/sb.h"

namespace client {

// A pseudo inode representing a mount point


const int MAX_NUM_ENTRIES=3;

class Session;

class MPInode: public Inode {
public:

	MPInode()
		: entries_count_(0)
	{ 
		pthread_mutex_init(&mutex_, NULL);
	}
	
	int Init(Session* session, uint64_t ino) { return 0; }
	int Open(Session* session, const char* path, int flags) { return 0; }
	int Write(Session* session, char* src, uint64_t off, uint64_t n) { return 0; }
	int Read(Session* session, char* dst, uint64_t off, uint64_t n) { return 0; }
	int Lookup(Session* session, const char* name, Inode** inode);
	int Link(Session* session, const char* name, Inode* inode, bool overwrite);
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) { assert(0); }
	int Unlink(client::Session* session, const char* name) { assert(0); }
	int Publish(client::Session* session) { return 0; }
	SuperBlock* GetSuperBlock() { return NULL; };
	void SetSuperBlock(SuperBlock* sb) { return; };

	struct Entry {
		char   name_[64];
		Inode* inode_;
	};

private:
	Inode*      parent_;
	Entry       entries_[MAX_NUM_ENTRIES];
	int         entries_count_;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_MPINODE_H
