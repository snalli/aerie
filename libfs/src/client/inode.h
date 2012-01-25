#ifndef __STAMNOS_FS_CLIENT_INODE_H
#define __STAMNOS_FS_CLIENT_INODE_H

#include <stdint.h>
#include <stdio.h>
#include "common/types.h"
#include "common/pnode.h"
#include "client/const.h"
#include "dpo/base/client/proxy.h"

namespace client {

class Session;     // forward declaration
class SuperBlock;  // forward declaration

class Inode {
public:
	Inode();

	virtual int Open(client::Session* session, const char* path, int flags) = 0;
	virtual int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	virtual int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	virtual int Lookup(client::Session* session, const char* name, Inode** ipp) = 0;
	virtual int Link(client::Session* session, const char* name, Inode* inode, bool overwrite) = 0;
	virtual int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) = 0;
	virtual int Unlink(client::Session* session, const char* name) = 0;

	virtual int nlink() = 0;
	virtual int set_nlink(int nlink) = 0;

	virtual int Publish(client::Session* session) = 0;

	client::SuperBlock* GetSuperBlock() { return sb_;}
	void SetSuperBlock(client::SuperBlock* sb) {sb_ = sb;}
	
	dpo::common::ObjectId oid() {
		if (ref_) {
			return ref_->obj()->oid();	
		}
		return dpo::common::ObjectId(0);
	}

	InodeNumber ino() {
		return oid().u64();
	}

	/* non-polymorphic functions */
	int Get();
	int Put();
	int Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode); 
	int Lock(::client::Session* session, lock_protocol::Mode mode); 
	int Unlock(::client::Session* session);
	int type() {
		return type_;
	}
	int fs_type() {
		return fs_type_;
	}

	dpo::common::ObjectProxyReference* ref_;     // reference to the persistent object container
//protected:
	Pnode*                             pnode_;   // pointer to the immutable persistent inode structure
	//! process-wide mutex; used for synchronizing access to the
	//! volatile inode metadata
	pthread_mutex_t                    mutex_;
	//! dynamic reference count; number of objects referencing the
	//! volatile inode object
	int                                refcnt_; 
	int                                fs_type_; // file system type this inode belongs to
	int                                type_;    // type of this inode
	client::SuperBlock*                sb_;      // volatile file system superblock
};


} // namespace client


#endif // __STAMNOS_FS_CLIENT_INODE_H
