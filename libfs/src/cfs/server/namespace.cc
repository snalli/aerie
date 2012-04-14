#include "cfs/server/namespace.h"
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
#include "cfs/server/sb.h"
#include "cfs/server/session.h"
#include "cfs/server/dir_inode.h"
#include "cfs/server/file_inode.h"
#include "bcs/bcs.h"

namespace server {

const int NAMESIZ = 128;
const int KERNEL_SUPERBLOCK = 0x1;


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


// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Inode is locked at mode lock_mode
int
NameSpace::Namex(Session* session, const char *cpath,  
                 bool nameiparent, char* name, InodeNumber* inop)
{
	char*       path = const_cast<char*>(cpath);
	DirInode    dinode;
	FileInode   finode;
	Inode*      ip;
	DirInode*   dp;
	DirInode*   dpn;
	FileInode*  fp;
	FileInode*  fpn;
	InodeNumber next_ino;
	int         ret;
	char*       old_name;

	dbg_log (DBG_INFO, "Namex: %s\n", path);
	
	if (*path == '/') {
		ip = DirInode::Load(session, root_ino_, &dinode);
	} else {
		//inode = cwd_;
	}

	while ((path = SkipElem(path, name)) != 0) {
		if (ip->type() != kDirInode) {
			*inop = 0;
			return -E_INVAL;
		}
		if (nameiparent && *path == '\0') {
			// stop one level early
			*inop = ip->ino();
			return E_SUCCESS;
		}
		dp = static_cast<DirInode*>(ip); // I know it's a directory inode as we did 
		                                 // the test when entering the block
		if ((ret = dp->Lookup(session, name, &next_ino)) < 0) {
			*inop = 0;
			return ret;
		}
		if (Inode::type(next_ino) == kDirInode) {
			ip = DirInode::Load(session, next_ino, &dinode);
		} else {
			ip = FileInode::Load(session, next_ino, &finode);
		}
	}
	if (nameiparent) {
		return -E_INVAL;
	}
	*inop = ip->ino();
	return E_SUCCESS;
}


int
NameSpace::Nameiparent(Session* session, const char* path, char* name, InodeNumber* ino)
{
	int ret; 

	ret = Namex(session, path, true, name, ino);
	return ret;
}


int
NameSpace::Namei(Session* session, const char* path, InodeNumber* ino)
{
	int  ret; 
	char name[128];

	ret = Namex(session, path, false, name, ino);
	return ret;
}

} // namespace server
