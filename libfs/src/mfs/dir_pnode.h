//! \file
//! Definition of the directory persistent inode structure stored in 
//! storage class memory
//!


// TODO: move factory code into a separate class/file

#ifndef __MFS_DIRECTORY_PERSISTENT_INODE_H_JAK129
#define __MFS_DIRECTORY_PERSISTENT_INODE_H_JAK129

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "common/pnode.h"
#include "mfs/mfs_i.h"
#include "mfs/hashtable.h"
#include "common/debug.h"
#include "common/util.h"

namespace mfs {

template<typename Session>
class DirPnode: public stm::object {
public:
	static DirPnode* Load(uint64_t ino) {
		return reinterpret_cast<DirPnode*>(ino);
	}
	
	DirPnode()
		: ht_(NULL),
		  self_(0),
		  parent_(0)
	{ 
		  nlink_ = 0;
	}

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		
		if (session->smgr_->Alloc(session, nbytes, typeid(DirPnode<Session>), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	int Lookup(Session* session, const char* name, uint64_t* ino);
	int Link(Session* session, const char* name, uint64_t ino);
	int Unlink(Session* session, const char* name);

//private:
	uint64_t            self_;    // entry '.'
	uint64_t            parent_;  // entry '..'
	HashTable<Session>* ht_;      // entries
};


template<typename Session>
int 
DirPnode<Session>::Lookup(Session* session, const char* name, uint64_t* ino)
{
	if (name[0] == '\0') {
		return -1;
	}	

	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			if (self_ == 0) {
				return -E_EXIST;
			}
			*ino = self_;
			return E_SUCCESS;
		case 2: // '..'
			if (self_ == 0) {
				return -E_EXIST;
			}
			*ino = parent_;
			return E_SUCCESS;
	}

	if (ht_) {
		return ht_->Search(session, name, strlen(name)+1, ino);
	}

	return -1;
}


template<typename Session>
int 
DirPnode<Session>::Link(Session* session, const char* name, uint64_t ino)
{
	uint64_t lino;

	if (name[0] == '\0') {
		return -1;
	}	
	
	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			self_ = ino;
			return 0;
		case 2: // '..'
			parent_ = ino;
			return 0;
	}
	
	if (!ht_) {
		ht_ = new(session) HashTable<Session>(); 
		if (!ht_) {
			return -1;
		}	
	}
	if (ht_->Search(session, name, strlen(name)+1, &lino)==0) {
		return -E_EXIST;
	}
	return ht_->Insert(session, name, strlen(name)+1, ino);
}


template<typename Session>
int 
DirPnode<Session>::Unlink(Session* session, const char* name)
{
	if (name[0] == '\0') {
		return -1;
	}	
	
	// handle special cases '.' and '..'
	switch (str_is_dot(name)) {
		case 1: // '.'
			self_ = 0;
			return 0;
		case 2: // '..'
			parent_ = 0;
			return 0;
	}

	if (!ht_) {
		return -1;
	}
	return ht_->Delete(session, name, strlen(name)+1);
}

} // namespace mfs

#endif // __MFS_DIRECTORY_PERSISTENT_INODE_H_JAK129
