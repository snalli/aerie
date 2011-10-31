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

//TODO: pathname resolution against current working directory, chdir

//TODO: extent the namei API to take as argument the type of lock on the resolved inode
// or expose to the caller what locks are held when the call is returned, and what locks
// the caller should acquire.

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
				Inode* root_inode = sb->GetRootInode();
				assert(inode->Insert(session, name, root_inode) == 0);	
				if (root_inode->Link(session, "..", inode, false) != 0) {
					return -1; // superblock already mounted
				}
				break;
			} else {
				// create mount point component
				inode_next = new MPInode();
				assert(inode->Insert(session, name, inode_next) == 0);	
				assert(inode_next->Insert(session, ".", inode_next) == 0);	
				assert(inode_next->Insert(session, "..", inode) == 0);	
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


// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// FIXME: Release inode when done (putinode) 
// FIXME: what locks do we hold when calling and when returning from this function???
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
		if (nameiparent && *path == '\0') {
			// Stop one level early.
			// FIXME: release lock?
			goto done;
		}

		printf("NameSpace::Namex [%p].Lookup(%s)\n", inode, name);
		if ((ret = inode->Lookup(session, name, &inode_next)) < 0) {
			return ret;
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
