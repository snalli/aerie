#ifndef __STAMNOS_FS_CLIENT_INODE_H
#define __STAMNOS_FS_CLIENT_INODE_H

#include <stdint.h>
#include <stdio.h>
#include "common/types.h"
#include "common/pnode.h"
#include "client/const.h"
#include "dpo/client/hlckmgr.h"


namespace client {

extern HLockManager* global_hlckmgr;

class Session;
class SuperBlock;

//FIXME: class Inode: public stm::ObjectProxy<Inode, Pnode> {
class Inode {
public:
	Inode();
	Inode(SuperBlock* sb, Pnode* pnode, InodeNumber ino);

	virtual int Init(client::Session* session, InodeNumber ino) = 0;
	virtual int Open(client::Session* session, const char* path, int flags) = 0;
	virtual int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	virtual int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	virtual int Lookup(client::Session* session, const char* name, Inode** ipp) = 0;
	virtual int Link(client::Session* session, const char* name, Inode* inode, bool overwrite) = 0;
	virtual int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) = 0;
	virtual int Unlink(client::Session* session, const char* name) = 0;

	virtual int Publish(client::Session* session) = 0;

	client::SuperBlock* GetSuperBlock() { return sb_;}
	void SetSuperBlock(client::SuperBlock* sb) {sb_ = sb;}
	
	InodeNumber ino() { return ino_; };
	void SetInodeNumber(InodeNumber ino) { ino_ = ino; };

	int nlink() { printf("Inode: %llu, nlink_ = %d\n", ino_, nlink_); return nlink_; }
	int set_nlink(int nlink) { printf("Inode: %llu, set_nlink_ = %d\n", ino_, nlink); nlink_ = nlink; return 0;}

	int Get();
	int Put();
	int Lock(Inode* parent_inode, lock_protocol::Mode mode); 
	int Lock(lock_protocol::Mode mode); 
	int Unlock();

	int type() {
		//FIXME: where do we store the type?
		return client::type::kDirInode;
	}

//protected:
	Pnode*              pnode_;             // pointer to the immutable persistent inode structure
	//! process-wide mutex; used for synchronizing access to the
	//! volatile inode metadata
	pthread_mutex_t     mutex_;
	//! dynamic reference count; number of objects referencing the
	//! volatile inode object
	int                 refcnt_; 
	//! hard link count
	int                 nlink_;
	client::SuperBlock* sb_;                // volatile file system superblock
	InodeNumber         ino_;
	//! system-wide public lock; used for inter- and intra process 
	//! synchronization to the persistent inode data structure
	client::Lock*       lock_;  

};


} // namespace client


#endif // __STAMNOS_FS_CLIENT_INODE_H
