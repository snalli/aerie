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
#include <typeinfo>
#include "common/util.h"
#include "common/hrtime.h"
#include "server/api.h"
#include "client/client_i.h"
#include "client/mpinode.h"
#include "client/hlckmgr.h"
#include "client/sb.h"
#include "common/debug.h"

#include <typeinfo>


//TODO: Optimization: When resolving a pathname (Namex), use LookupFast API 
// instead of Lookup and revert to Lookup only when the inode is mutated

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


//FIXME: Replace Insert with Link
int
NameSpace::Mount(Session* session, const char* const_path, SuperBlock* sb)
{
	char     name[128];
	MPInode* mpnode;
	MPInode* mpnode_next;
	int      ret;
	char*    path = const_cast<char*>(const_path);
	
	mpnode = root_;
	while((path = SkipElem(path, name)) != 0) {
		if ((ret = mpnode->Lookup(session, name, (Inode**) &mpnode_next)) < 0) {
			if (*path == '\0') {
				// end of path name -- mount superblock
				assert(mpnode->Insert(session, name, sb->GetRootInode()) == 0);	
				break;
			} else {
				// create mount point component
				mpnode_next = new MPInode();
				assert(mpnode->Insert(session, name, mpnode_next) == 0);	
			}
		}
		mpnode = mpnode_next;
	}

	return 0;
}

int
NameSpace::Unmount(Session* session, char* name)
{
	return 0;
}


// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
int
NameSpace::Namex(Session* session, const char *cpath, bool nameiparent, 
                 char* name, Inode** inodep)
{
	char*       path = const_cast<char*>(cpath);
	Inode*      inode;
	Inode*      inode_next;
	int         ret;
	SuperBlock* sb;
	
	printf("NameSpace::Namex(%s)\n", cpath);
	
	inode = root_;
	inode->Lock(lock_protocol::Mode::IXSL);
	while ((path = SkipElem(path, name)) != 0) {
		printf("name=%s\n", name);
retry:	
		if (!inode) {
			printf("NameSpace::Namex(%s): DONE 1\n", cpath);
			return -1;
		}
		/*
		if (nameiparent && *path == '\0') {
			// Stop one level early.
			//TODO: unlock inode after lookup and lock the next
			// follow superblock's root inode if parent is mount point
			if (typeid(*inode) == typeid(MPInode)) {
				if ((ret = inode->Lookup(session, name, &inode_next)) == -2) {
					MPInode* mpnode = dynamic_cast<MPInode*>(inode);
					sb = mpnode->GetSuperBlock();
					if ((uint64_t) sb != KERNEL_SUPERBLOCK) {
						inode = sb->GetRootInode();
						if (inode->ino_) {
							global_hlckmgr->Acquire(inode->ino_, lock_protocol::Mode::IXSL, 0);
						}
					}
				}
			}
			goto done;
		}
		*/
		if (nameiparent && *path == '\0') {
			// Stop one level early.
			goto done;
		}

		//FIXME: Lock (latch) inode before looking up/then unlock (i.e. do spider locking)
		//FIXME: Release inode when done 
		if ((ret = inode->Lookup(session, name, &inode_next)) < 0) {
			printf("NameSpace::Namex(%s): Lookup: %s: ret=%d\n", cpath, name, ret);
			/*
			if (ret == -2) {
				MPInode* mpnode = dynamic_cast<MPInode*>(inode);
				sb = mpnode->GetSuperBlock();
				if ((uint64_t) sb == KERNEL_SUPERBLOCK) {
					//printf("NameSpace::Namei(%s): DONE KERNEL_SUPERBLOCK\n", cpath);
					return -E_KVFS;
				} else {
					// follow superblock's root inode and lookup 'name' in the root inode
					printf("NameSpace::Namex(%s): follow superblock root \n", cpath);
					inode_next = sb->GetRootInode();
					if (inode_next->ino_) {
						global_hlckmgr->Acquire(inode_next->ino_, lock_protocol::Mode::IXSL, 0);
					}
					inode = inode_next;
					goto retry;
				}
			} else {
				//printf("NameSpace::Namei(%s): not found\n", cpath);
				return ret;
			}	
			*/
			return -E_KVFS;
		}
		printf("name=%s, inode_next=%p\n", name, inode_next);
		// spider locking
		inode_next->Lock(lock_protocol::Mode::IXSL);
		inode->Unlock();
		inode = inode_next;
	}	
	if (nameiparent) {
		return -1;
	}
done:
	*inodep = inode;
	return 0;
}


//TODO: optimization: when namex uses immutable for lookup, nameiparent should be able
//to ask for a mutable inode
int
NameSpace::Nameiparent(Session* session, const char* path, char* name, Inode** inodep)
{
	int ret; 

	ret = Namex(session, path, true, name, inodep);
	return ret;
}


int
NameSpace::Namei(Session* session, const char* path, Inode** inodep)
{
	int  ret; 
	char name[128];

	ret = Namex(session, path, false, name, inodep);
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
