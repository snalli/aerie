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
	
	int Write(Session* session, char* src, uint64_t off, uint64_t n) { return 0; }
	int Read(Session* session, char* dst, uint64_t off, uint64_t n) { return 0; }
	int Lookup(Session* session, const char* name, int flags, Inode** inode);
	int xLookup(Session* session, const char* name, int flags, Inode** inode) { return 0; }
	int Link(Session* session, const char* name, Inode* inode, bool overwrite);
	int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) { assert(0); }
	int Unlink(client::Session* session, const char* name) { assert(0); }
	
	int nlink() { return 0; }
	int set_nlink(int nlink) { return 0; }

	int Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode); 
	int Lock(::client::Session* session, lock_protocol::Mode mode); 
	int Unlock(::client::Session* session);
	int xOpenRO(::client::Session* session) { return 0; } 
	
	int ioctl(::client::Session* session, int request, void* info) { return 0; }

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
