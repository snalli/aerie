#ifndef _CLIENT_INODE_H_JAK129
#define _CLIENT_INODE_H_JAK129

#include <stdint.h>
#include <stdio.h>
#include "client/hlckmgr.h"

namespace client {

extern HLockManager* global_hlckmgr;

typedef uint64_t InodeNumber;

class Session;
class SuperBlock;


//FIXME: why do we have Insert and Link in the API???
class Inode {
public:
	virtual int Init(client::Session* session, InodeNumber ino) = 0;
	virtual int Open(client::Session* session, const char* path, int flags) = 0;
	virtual int Write(client::Session* session, char* src, uint64_t off, uint64_t n) = 0;
	virtual int Read(client::Session* session, char* dst, uint64_t off, uint64_t n) = 0;
	virtual int Lookup(client::Session* session, const char* name, Inode** inode) = 0;
	virtual int LookupFast(client::Session* session, const char* name, Inode* inode) = 0;
	virtual int Insert(client::Session* session, const char* name, Inode* inode) = 0;
	virtual int Link(client::Session* session, const char* name, Inode* inode, bool overwrite) = 0;
	virtual int Link(client::Session* session, const char* name, uint64_t ino, bool overwrite) = 0;
	virtual int Unlink(client::Session* session, const char* name) = 0;

	virtual int Publish(client::Session* session) = 0;

	virtual client::SuperBlock* GetSuperBlock() = 0;
	virtual void SetSuperBlock(client::SuperBlock* sb) = 0;

	virtual InodeNumber GetInodeNumber() { return ino_; };
	virtual void SetInodeNumber(InodeNumber ino) { ino_ = ino; };

	int Get();
	int Put();
	int Lock(lock_protocol::Mode mode); 
	int Unlock();

//protected:
	//! process-wide mutex; used for synchronizing access to the
	//! volatile inode metadata
	pthread_mutex_t mutex_;
	//! dynamic reference count; number of objects referencing the
	//! volatile inode object
	int             refcnt_; 
	InodeNumber     ino_;
	//! system-wide public lock; used for inter- and intra process 
	//! synchronization to the persistent inode data structure
	client::Lock*   lock_;  

};


//FIXME: when creating the inode, inode manager assigns a public lock if 
// needed. if there is a public lock, then we acquire it otherwise we acquire the
// private lock
inline int
Inode::Lock(lock_protocol::Mode mode)
{
	if (ino_) {
		return global_hlckmgr->Acquire(ino_, mode, 0);
	}
	return 0;
}


inline int
Inode::Unlock()
{
	if (ino_) {
		return global_hlckmgr->Release(ino_);
	}
	return 0;
}

inline
int Inode::Get() 
{ 
	printf("Inode::Get mutex=%p\n", &mutex_);
	pthread_mutex_lock(&mutex_);
	refcnt_++; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}

inline
int Inode::Put() 
{ 
	pthread_mutex_lock(&mutex_);
	assert(refcnt_>0); 
	refcnt_--; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}


/*

class InodeImmutable: public Inode {
public:
	
	static InodeImmutable* ino2obj(uint64_t ino) {
		return reinterpret_cast<InodeImmutable*> (ino);
	}

private:


};


*/

} // namespace client


#endif /* _CLIENT_INODE_H_JAK129 */
