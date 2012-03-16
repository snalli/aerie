#ifndef __STAMNOS_FS_CLIENT_INODE_H
#define __STAMNOS_FS_CLIENT_INODE_H

#include <stdint.h>
#include <stdio.h>
#include "common/types.h"
#include "client/const.h"
#include "ssa/main/client/proxy.h"

namespace client {

class Session;     // forward declaration
class SuperBlock;  // forward declaration

class Inode {
public:
	Inode();

	virtual int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	virtual int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	virtual int Lookup(client::Session* session, const char* name, int flags, Inode** ipp) = 0;
	virtual int xLookup(client::Session* session, const char* name, int flags, Inode** ipp) = 0;
	virtual int Link(client::Session* session, const char* name, Inode* inode, bool overwrite) = 0;
	virtual int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) = 0;
	virtual int Unlink(client::Session* session, const char* name) = 0;
	virtual int Sync(::client::Session* session) = 0; 

	virtual int nlink() = 0;
	virtual int set_nlink(int nlink) = 0;

	virtual int Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode) = 0; 
	virtual int Lock(::client::Session* session, lock_protocol::Mode mode) = 0; 
	virtual int Unlock(::client::Session* session) = 0;
	virtual int xOpenRO(::client::Session* session) = 0; 

	/** 
	 * A generic interface method for functionality that doesn't fall under
	 * any specific method
	 */
	virtual int ioctl(::client::Session* session, int request, void* info) = 0;

	ssa::common::ObjectId oid() {
		if (ref_) {
			return ref_->proxy()->oid();	
		}
		return ssa::common::ObjectId(0);
	}

	InodeNumber ino() {
		return oid().u64();
	}

	/* non-polymorphic functions */
	int Get();
	int Put();
	int type() {
		return type_;
	}
	int fs_type() {
		return fs_type_;
	}

//protected:
	ssa::common::ObjectProxyReference* ref_;     // reference to the persistent object container
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
