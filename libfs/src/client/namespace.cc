#include "client/namespace.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <stack>
#include <typeinfo>
#include "common/util.h"
#include "common/hrtime.h"
#include "server/api.h"
#include "client/client_i.h"
#include "client/mpinode.h"
#include "client/inode.h"
#include "client/sb.h"
#include "dpo/base/client/hlckmgr.h"
//#include "dpo/client/stm.h"
#include "common/debug.h"

#include <typeinfo>

//TODO: pathname resolution against current working directory, chdir
//TODO: integrate LockInode
//TODO: directory operations update object version
//TODO: rename
//TODO: test rename

//TODO: extent the namei API to take as argument the type of lock on the resolved inode
// or expose to the caller what locks are held when the call is returned, and what locks
// the caller should acquire.

// LOCK PROTOCOL
// 
// read-only file:
//
// need to locally lock each inode along the path at level SL. 
// if the local lock has no parent lock then you need to 
// acquire a base SR lock on the inode.
// 
// read-write file:
//
// need to locally lock each inode except the file inode at IXSL
// if the local lock has no parent lock then you need to 
// acquire a base XR lock on the inode. 
// locally lock the file inode at XL
//

using namespace client;

const int NAMESIZ = 128;
const int KERNEL_SUPERBLOCK = 0x1;


NameSpace::NameSpace(rpcc* c, unsigned int principal_id, const char* namespace_name)
	: client_(c), 
	  principal_id_(principal_id)
{
	strcpy(namespace_name_, namespace_name);
}

//! Copy the next path element from path into name.
//! Return a pointer to the element following the copied one.
//! The returned path has no leading slashes,
//! so the caller can check *path=='\0' to see if the name is the last one.
//! If no name to remove, return 0.
//
//! Examples:
//!   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//!   skipelem("///a//bb", name) = "bb", setting name = "a"
//!   skipelem("a", name) = "", setting name = "a"
//!   skipelem("", name) = skipelem("////", name) = 0
//!
static char*
SkipElem(char *path, char *name)
{
	char* s;
	int   len;

	while(*path == '/') {
		path++;
	}	
	if(*path == 0) {
		return 0;
	}	
	s = path;
	while(*path != '/' && *path != 0) {
		path++;
	}
	len = path - s;
	if(len >= NAMESIZ) {
		memmove(name, s, NAMESIZ);
	} else {
		memmove(name, s, len);
		name[len] = 0;
	}
	while(*path == '/') {
		path++;
	}	
	return path;
}




int
NameSpace::Init(Session* session) 
{
	root_ = new MPInode;
	Mount(session, "/", (SuperBlock*) KERNEL_SUPERBLOCK);
}


int
NameSpace::Lookup(Session* session, const char* name, void** obj)
{
	return 0;
}


int
NameSpace::Mount(Session* session, const char* const_path, SuperBlock* sb)
{
	char   name[128];
	Inode* inode;
	Inode* inode_next;
	int    ret;
	char*  path = const_cast<char*>(const_path);
	
	inode = root_;
	while((path = SkipElem(path, name)) != 0) {
		if ((ret = inode->Lookup(session, name, (Inode**) &inode_next)) < 0) {
			if (typeid(*inode) != typeid(client::MPInode)) {
				// cannot mount a filesystem on non-mount-point pseudo-inode  
				return -1;
			}
			if (*path == '\0') {
				// reached end of path -- mount superblock
				//FIXME: SUPERBLOCK
				//Inode* root_inode = sb->RootInode();
				Inode* root_inode;
				printf("ROOT INODE: %p\n", root_inode);
				assert(inode->Link(session, name, root_inode, false) == 0);	
				if (root_inode->Link(session, "..", inode, false) != 0) {
					return -1; // superblock already mounted
				}
				break;
			} else {
				// create mount point component
				inode_next = new MPInode();
				assert(inode->Link(session, name, inode_next, false) == 0);	
				assert(inode_next->Link(session, ".", inode_next, false) == 0);	
				assert(inode_next->Link(session, "..", inode, false) == 0);	
			}
		}
		inode = inode_next;
	}

	return 0;
}


int
NameSpace::Unmount(Session* session, char* name)
{
	return 0;
}


// caller does not hold any lock 
// 
// it relies on a read-only optimistic memory transaction to atomically 
// traverse the chain up to the root and then acquire locks down till the inode
int
NameSpace::LockInode(Session* session, Inode* inode, lock_protocol::Mode lock_mode)
{
// FIXME
#if 0 
	std::vector<Inode*> inode_chain;   // treated as a stack
	std::vector<Inode*> locked_inodes; 
	Inode*              tmp_inode;
	Inode*              parent_inode;
	int                 retries = 0;
	int                 ret;

	STM_BEGIN()
		// transaction does not automatically release any acquired locks 
		// so we need to do this manually
		inode_chain.clear();
		for (std::vector<Inode*>::iterator it = locked_inodes.begin();
		     it != locked_inodes.end();
			 it++)
		{
			(*it)->Unlock();
		}
		locked_inodes.clear();
		for (tmp_inode = inode; tmp_inode != root_; tmp_inode = parent_inode) {
			inode_chain.push_back(tmp_inode);
			tmp_inode = tmp_inode->xOpenRO();
			ret = tmp_inode->Lookup(session, "..", &parent_inode);
			if (ret != E_SUCCESS) {

			}
			STM_ABORT_IF_INVALID() // read-set consistency
		}
		// acquire locks on the inodes in the reverse order they appear 
		// in the list 
		parent_inode = NULL;
		for (std::vector<Inode*>::iterator it = inode_chain.end();
		     it != locked_inodes.begin();
			 it--)
		{
			if (*it == inode) {
				if (parent_inode) {
					assert((*it)->Lock(parent_inode, lock_mode) == 0);
				} else {
					assert(*it == root_);
					assert((*it)->Lock(lock_mode) == 0);
				}
			} else {
				if (parent_inode) {
					assert((*it)->Lock(parent_inode, lock_protocol::Mode::IX) == 0);
				} else {
					assert(*it == root_);
					assert((*it)->Lock(lock_protocol::Mode::IX) == 0);
				}
			}
			parent_inode = *it;
		}
	STM_END()
#endif
}


// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
int
NameSpace::Namex(Session* session, const char *cpath, lock_protocol::Mode lock_mode, 
                 bool nameiparent, char* name, Inode** inodep)
{
	char*       path = const_cast<char*>(cpath);
	Inode*      inode;
	Inode*      inode_next;
	Inode*      inode_prev = NULL;
	int         ret;
	SuperBlock* sb;
	char*       old_name;

	
	if (!path) {
		return -E_INVAL;
	}

	printf("NameSpace::Namex(%s)\n", path);

	// find whether absolute or relative path
	if (*path == '/') {
		inode = root_;
	} else {
		// TODO: if cwd is not assigned a lock then we need to re-acquire locks 
		// on the along the chain from root to cwd. the lock manager should detect this
		// case and bootstrap. LockInode?
		// idup(cwd_inode)
		assert(0 && "TODO");
	}

	// boundary condition: get the lock on the first inode
	path = SkipElem(path, name);
	if (nameiparent && path != 0 && *path == '\0') {
		inode->Lock(lock_mode);
		inode->Get();
		goto done;
	} else {
		inode->Lock(lock_protocol::Mode::IXSL);
		inode->Get();
	}
	
	// consume the rest of the path by acquiring a lock on each inode in a spider
	// locking fashion. spider locking is necessary because acquiring an
	// hierarchical lock on an inode requires having the parent locked.
	// if we encounter a .. then we move up the hierarchy but we don't do spider 
	// locking. however the client may no longer own the lock on the .. if another
	// client asked the lock. in this case we need to boostrap the lock by
	// acquiring locks along the chain from the root to the inode ..
	while (path != 0) {
		printf("Namex: inode=%p (ino=%lu), name=%s\n", inode, inode->ino(), name);
		if ((ret = inode->Lookup(session, name, &inode_next)) < 0) {
			inode->Put();
			inode->Unlock();
			return ret;
		}
		old_name = name;
		path = SkipElem(path, name);
		if (nameiparent && path != 0 && *path == '\0') {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				inode->Unlock();
				assert(inode_next->Lock(lock_mode) == E_SUCCESS);
			} else {
				// spider locking
				assert(inode_next->Lock(inode, lock_mode) == E_SUCCESS);
				inode->Unlock();
			}
			inode->Put();
			inode = inode_next;
			printf("Namex(nameiparent=true): inode=%p\n", inode);
			goto done;
		} else {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				inode->Unlock();
				if (inode_next->Lock(lock_protocol::Mode::IXSL) != E_SUCCESS) {
					LockInode(session, inode_next, lock_protocol::Mode::IXSL);
				}
			} else {
				assert(inode_next->Lock(inode, lock_protocol::Mode::IXSL) == E_SUCCESS);
				inode->Unlock();
			}
			inode->Put();
			inode = inode_next;
		}
	}
	printf("Namex: inode=%p\n", inode);

done:
	*inodep = inode;
	return 0;
}


int
NameSpace::Nameiparent(Session* session, const char* path, lock_protocol::Mode lock_mode, char* name, Inode** inodep)
{
	int ret; 

	ret = Namex(session, path, lock_mode, true, name, inodep);
	return ret;
}


int
NameSpace::Namei(Session* session, const char* path, lock_protocol::Mode lock_mode, Inode** inodep)
{
	int  ret; 
	char name[128];

	ret = Namex(session, path, lock_mode, false, name, inodep);
	return ret;
}


int
NameSpace::Link(Session* session, const char *name, void *obj)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
}


int
NameSpace::Unlink(Session* session, const char *name)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
}
