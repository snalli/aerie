//! \file
//! Definition of the directory persistent inode structure stored in 
//! storage class memory
//!

#ifndef __MFS_DIRECTORY_PERSISTENT_INODE_H_JAK129
#define __MFS_DIRECTORY_PERSISTENT_INODE_H_JAK129


#include <stdint.h>
#include <typeinfo>
#include "client/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/pnode.h"
#include "mfs/hashtable.h"
#include "common/debug.h"

namespace mfs {

template<typename Session>
class DirPnode: public Pnode {
public:
	static DirPnode* Load(uint64_t ino) {
		return reinterpret_cast<DirPnode*>(ino);
	}
	
	DirPnode()
		: ht_(NULL),
		  self_(0),
		  parent_(0)
	{ }

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		
		if (session->sm->Alloc(session, nbytes, typeid(DirPnode<Session>), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	int Lookup(Session* session, const char* name, uint64_t* ino);
	int Link(Session* session, const char* name, uint64_t ino);

//private:
	uint64_t            self_;    // entry '.'
	uint64_t            parent_;  // entry '..'
	HashTable<Session>* ht_;      // entries
};


template<typename Session>
inline int 
DirPnode<Session>::Lookup(Session* session, const char* name, uint64_t* ino)
{
	if (name[0] == '\0') {
		return -1;
	}	

	// handle special cases '.' and '..'
	if (name[1] == '\0') {
		if (name[0] == '.') {
			*ino = self_;
			return 0;
		} 
	} else if (name[2] == '\0') {
		if (name[0] == '.' && name[1] == '.') {
			*ino = parent_;
			return 0;
		} 
	}

	if (ht_) {
		return ht_->Search(session, name, strlen(name)+1, ino);
	}

	return -1;
}


template<typename Session>
inline int 
DirPnode<Session>::Link(Session* session, const char* name, uint64_t ino)
{
	if (name[0] == '\0') {
		return -1;
	}	

	// handle special cases '.' and '..'
	if (name[1] == '\0') {
		if (name[0] == '.') {
			self_ = ino;
			return 0;
		} 
	} else if (name[2] == '\0') {
		if (name[0] == '.' && name[1] == '.') {
			parent_ = ino;
			return 0;
		} 
	}
	
	if (!ht_) {
		ht_ = new(session) HashTable<Session>(); 
		if (!ht_) {
			return -1;
		}	
	}
	ht_->Insert(session, name, strlen(name)+1, ino);

	return 0;
}

} // namespace mfs
#endif // __MFS_DIRECTORY_PERSISTENT_INODE_H_JAK129
