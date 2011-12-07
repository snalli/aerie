#ifndef _CLIENT_INODE_H_JAK129
#define _CLIENT_INODE_H_JAK129

#include <stdint.h>
#include <stdio.h>
#include "common/types.h"
#include "common/pnode.h"
#include "client/const.h"
#include "client/hlckmgr.h"
#include "client/stm.h"


namespace client {

extern HLockManager* global_hlckmgr;

class Session;
class SuperBlock;

class BaseInode {
public:
	Inode();
	Inode(SuperBlock* sb, Pnode* pnode, InodeNumber ino);

	int Init(client::Session* session, InodeNumber ino) = 0;
	int Open(client::Session* session, const char* path, int flags) = 0;
	int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	int Lookup(client::Session* session, const char* name, Inode** inodep) = 0;
	int Link(client::Session* session, const char* name, Inode* inode, bool overwrite) = 0;
	int Unlink(client::Session* session, const char* name) = 0;
	int Update(client::Session* session) = 0;

	client::SuperBlock* GetSuperBlock() { return sb_;}
	void SetSuperBlock(client::SuperBlock* sb) {sb_ = sb;}
	
	InodeNumber ino() { return ino_; };
	void SetInodeNumber(InodeNumber ino) { ino_ = ino; };

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
	PnodeProxy*         pnode_;             // pointer to the persistent inode structure
	//! process-wide mutex; used for synchronizing access to the
	//! volatile inode metadata
	pthread_mutex_t     mutex_;
	//! dynamic reference count; number of objects referencing the
	//! volatile inode object
	int                 refcnt_; 
	client::SuperBlock* sb_;                // volatile file system superblock
	InodeNumber         ino_;
	//! system-wide public lock; used for inter- and intra process 
	//! synchronization to the persistent inode data structure
	client::Lock*       lock_;  

};


template<class PnodeProxy>
class Inode: public BaseInode {
public:
	Inode();
	Inode(SuperBlock* sb, Pnode* pnode, InodeNumber ino);

	int Init(client::Session* session, InodeNumber ino) = 0;
	int Open(client::Session* session, const char* path, int flags) = 0;
	int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	int Lookup(client::Session* session, const char* name, Inode** inodep) = 0;
	int Link(client::Session* session, const char* name, Inode* inode, bool overwrite) = 0;
	int Unlink(client::Session* session, const char* name) = 0;
	int Update(client::Session* session) = 0;

	client::SuperBlock* GetSuperBlock() { return sb_;}
	void SetSuperBlock(client::SuperBlock* sb) {sb_ = sb;}
	
	InodeNumber ino() { return ino_; };
	void SetInodeNumber(InodeNumber ino) { ino_ = ino; };

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
	PnodeProxy*         pnode_;             // pointer to the persistent inode structure
	//! process-wide mutex; used for synchronizing access to the
	//! volatile inode metadata
	pthread_mutex_t     mutex_;
	//! dynamic reference count; number of objects referencing the
	//! volatile inode object
	int                 refcnt_; 
	client::SuperBlock* sb_;                // volatile file system superblock
	InodeNumber         ino_;
	//! system-wide public lock; used for inter- and intra process 
	//! synchronization to the persistent inode data structure
	client::Lock*       lock_;  

};



} // namespace client


#endif /* _CLIENT_INODE_H_JAK129 */
