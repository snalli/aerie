#ifndef _CLIENT_INODE_H_JAK129
#define _CLIENT_INODE_H_JAK129

#include <stdint.h>
#include <stdio.h>
#include "client/lckmgr.h"

namespace client {

extern LockManager* global_lckmgr;

typedef uint64_t InodeNumber;

class Session;
class SuperBlock;

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

	virtual client::SuperBlock* GetSuperBlock() = 0;
	virtual void SetSuperBlock(client::SuperBlock* sb) = 0;

	virtual InodeNumber GetInodeNumber() { return ino_; };
	virtual void SetInodeNumber(InodeNumber ino) { ino_ = ino; };

	int Lock(); 
	int Unlock();

protected:
	InodeNumber    ino_;
	client::Lock*  lock_;
};


inline int
Inode::Lock()
{
	//FIXME
	//global_lckmgr->Acquire(lock_);
}


inline int
Inode::Unlock()
{
	//FIXME
	//global_lckmgr->Release(lock_);
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
