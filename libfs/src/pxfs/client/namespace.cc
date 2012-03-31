#include "pxfs/client/namespace.h"
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
#include "ssa/main/client/stm.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/mpinode.h"
#include "pxfs/client/inode.h"
#include "pxfs/client/sb.h"
#include "pxfs/client/session.h"
#include "pxfs/common/publisher.h"
#include "bcs/bcs.h"


//TODO: pathname resolution against current working directory, chdir
//TODO: integrate LockInode
//TODO: directory operations update object version
//TODO: port rename
//TODO: test rename

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


NameSpace::NameSpace(const char* namespace_name)
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
		if ((ret = inode->Lookup(session, name, 0, (Inode**) &inode_next)) < 0) {
			if (typeid(*inode) != typeid(client::MPInode)) {
				// cannot mount a filesystem on non-mount-point pseudo-inode  
				return -1;
			}
			if (*path == '\0') {
				// reached end of path -- mount superblock
				Inode* root_inode = sb->RootInode();
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
// traverse the chain up to the root and then acquire locks down the path 
// until it finds the inode
int
NameSpace::LockInodeReverse(Session* session, Inode* inode, lock_protocol::Mode lock_mode)
{
	std::vector<Inode*> inode_chain;   // treated as a stack
	std::vector<Inode*> locked_inodes; 
	Inode*              tmp_inode;
	Inode*              parent_inode;
	int                 retries = 0;
	int                 ret;

	STM_BEGIN()
		// the transaction does not automatically release any acquired locks 
		// so we need to do this manually
		inode_chain.clear();
		for (std::vector<Inode*>::iterator it = locked_inodes.begin();
		     it != locked_inodes.end();
			 it++)
		{
			(*it)->Unlock(session);
		}
		locked_inodes.clear();
		// optimistically find all the inodes that appear in the path that begins
		// from the current inode up to the root inode
		for (tmp_inode = inode; tmp_inode != root_; tmp_inode = parent_inode) {
			inode_chain.push_back(tmp_inode);
			tmp_inode->xOpenRO(session);
			ret = tmp_inode->xLookup(session, "..", 0, &parent_inode);
			if (ret != E_SUCCESS) {

			}
			STM_ABORT_IF_INVALID() // read-set consistency
		}
		// acquire hierarchical locks on the inodes in the reverse order they 
		// appear in the list (i.e. root to leaf)
		parent_inode = NULL;
		for (std::vector<Inode*>::iterator it = inode_chain.end();
		     it != inode_chain.begin();
			 it--)
		{
			if (*it == inode) {
				if (parent_inode) {
					assert((*it)->Lock(session, parent_inode, lock_mode) == 0);
				} else {
					assert(*it == root_);
					assert((*it)->Lock(session, lock_mode) == 0);
				}
			} else {
				if (parent_inode) {
					assert((*it)->Lock(session, parent_inode, lock_protocol::Mode::IX) == 0);
				} else {
					assert(*it == root_);
					assert((*it)->Lock(session, lock_protocol::Mode::IX) == 0);
				}
			}
			parent_inode = *it;
		}
	STM_END()

	// now release all the locks except the lock on the inode we were asked to lock
	for (std::vector<Inode*>::iterator it = locked_inodes.begin();
		 it != locked_inodes.end();
		 it++)
	{
		if (*it != inode) {
			(*it)->Unlock(session);
		}
	}
	return E_SUCCESS;
}


// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Inode is locked at mode lock_mode
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
		// along the chain from root to cwd. the lock manager should detect this
		// case and bootstrap. use LockInode?
		// idup(cwd_inode)
		// TODO: 
		// try to acquire the cached lock. if we can't then we bootstrap
		// by calling LockInodeReverse to acquire locks in the reverse order 
		assert(0 && "TODO");
	}

	// boundary condition: get the lock on the first inode
	path = SkipElem(path, name);
	if (nameiparent && path != 0 && *path == '\0') {
		inode->Lock(session, lock_mode);
		inode->Get();
		goto done;
	} else {
		inode->Lock(session, lock_protocol::Mode::IXSL);
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
		if ((ret = inode->Lookup(session, name, 0, &inode_next)) < 0) {
			inode->Put();
			inode->Unlock(session);
			return ret;
		}
		old_name = name;
		path = SkipElem(path, name);
		if (nameiparent && path != 0 && *path == '\0') {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				inode->Unlock(session);
				assert(inode_next->Lock(session, lock_mode) == E_SUCCESS);
			} else {
				// spider locking
				assert(inode_next->Lock(session, inode, lock_mode) == E_SUCCESS);
				inode->Unlock(session);
			}
			inode->Put();
			inode = inode_next;
			printf("Namex(nameiparent=true): inode=%p\n", inode);
			goto done;
		} else {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				// Cannot do hierarchical locking on the reverse order (i.e. acquire 
				// parent under child) so we try to acquire the cached lock on .. 
				// by not providing a parent hint for ..
				// If this fails then we redo locking beginning from the root
				inode->Unlock(session);
				if (inode_next->Lock(session, lock_protocol::Mode::IXSL) != E_SUCCESS) {
					LockInodeReverse(session, inode_next, lock_protocol::Mode::IXSL);
				}
			} else {
				assert(inode_next->Lock(session, inode, lock_protocol::Mode::IXSL) == E_SUCCESS);
				inode->Unlock(session);
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
NameSpace::Link(Session* session, const char *oldpath, const char* newpath)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");

	//session->journal() << Publisher::Messages::LogicalOperation::Link(ino(), name, ip->ino());
/*
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;
  if((ip = namei(old)) == 0)
    return -1;
  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    return -1;
  }
  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);
  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  return -1;
}
*/
}


int
NameSpace::Unlink(Session* session, const char *pathname)
{
	char          name[128];
	Inode*        dp;
	Inode*        ip;
	SuperBlock*   sb;
	int           ret;

	dbg_log (DBG_INFO, "Unlink: %s\n", pathname);	

	// we do spider locking; when Nameiparent returns successfully, dp is 
	// locked for writing. we release the lock on dp after we get the lock
	// on its child
	if ((ret = global_namespace->Nameiparent(session, pathname, lock_protocol::Mode::XL, 
	                                         name, &dp)) < 0) 
	{
		return ret;
	}

	// Cannot unlink "." or "..".
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		dp->Put();
		dp->Unlock(session);
		return -E_INVAL;
	}

	if ((ret = dp->Lookup(session, name, 0, &ip)) != E_SUCCESS) {
		dp->Put();
		dp->Unlock(session);
		return ret;
	}
	
	// do we need recursive lock if ip is a file? 
	ip->Lock(session, dp, lock_protocol::Mode::XR); 
	assert(ip->nlink() > 0);

	if (ip->type() == kDirInode) {
		bool isempty;
		assert(ip->ioctl(session, 1, &isempty) == E_SUCCESS);
		if (!isempty) {
			ip->Put();
			ip->Unlock(session);
			dp->Put();
			dp->Unlock(session);
			return -E_NOTEMPTY;
		}
	}

	session->journal() << Publisher::Messages::LogicalOperation::Unlink(dp->ino(), name);
	
	assert(dp->Unlink(session, name) == E_SUCCESS);
	//FIXME: inode link/unlink should take care of the nlink
	if (ip->type() == kDirInode) {
		assert(dp->set_nlink(dp->nlink() - 1) == 0); // for child's backlink ..
	}

	assert(ip->set_nlink(ip->nlink() - 1) == 0); // for parent's forward link
	//FIXME: who deallocates the inode if nlink == 0 ???
	
	dp->Put();
	dp->Unlock(session);
	ip->Put();
	ip->Unlock(session);

	return E_SUCCESS;
}
