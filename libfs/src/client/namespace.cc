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
#include "client/sb.h"
#include "common/debug.h"

#include <typeinfo>

using namespace client;

const int NAMESIZ = 128;
const int KERNEL_SUPERBLOCK = 0x1;


NameSpace::NameSpace(rpcc* c, unsigned int principal_id, const char* namespace_name)
	: client_(c), 
	  principal_id_(principal_id)
{
	strcpy(namespace_name_, namespace_name);
}

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
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
NameSpace::Init() 
{
	root_ = new MPInode;
	Mount("/", (SuperBlock*) KERNEL_SUPERBLOCK);
}


int
NameSpace::Lookup(const char* name, void** obj)
{
	return 0;
}

#if 0
int
NameSpace::Mount(const char* path, void* superblock)
{
	std::string        name_str;
	unsigned long long val;
	int                r;
	int                intret;

	name_str = std::string(path);
	val = (unsigned long long) superblock;

	intret = client_->call(RPC_NAMESPACE_MOUNT, principal_id_, name_str, val, r);
	if (intret) {
		return -intret;
	}

	return 0;
}


int
NameSpace::Unmount(const char* name)
{
	return 0;
}


#else 

int
NameSpace::Mount(const char* const_path, SuperBlock* sb)
{
	char     name[128];
	MPInode* mpnode;
	MPInode* mpnode_next;
	int      ret;
	char*    path = const_cast<char*>(const_path);
	
	printf("NameSpace::Mount(%s)\n", path);

	mpnode = root_;
	while((path = SkipElem(path, name)) != 0) {
		printf("NameSpace::Mount: elem=%s\n", name);
		if ((ret = mpnode->Lookup(name, (Inode**) &mpnode_next)) < 0) {
			mpnode_next = new MPInode();
			printf("mpnode_next=%p\n", mpnode_next);
			assert(mpnode->Insert(name, mpnode_next) == 0);	
			mpnode = mpnode_next;
			continue;
		}
		mpnode = mpnode_next;
	}
	mpnode->SetSuperBlock(sb);

	return 0;
}

int
NameSpace::Unmount(char* name)
{
	return 0;
}



#endif


//TODO: Optimization: Use LookupFast API instead of Lookup and revert to 
// Lookup only when the inode is mutated
int
NameSpace::Namex(const char *cpath, bool nameiparent, char* name, Inode** inodep)
{
	char*       path = const_cast<char*>(cpath);
	Inode*      inode;
	Inode*      inode_next;
	int         ret;
	SuperBlock* sb;
	
	printf("NameSpace::Namex(%s)\n", cpath);
	
	inode = root_;
	while ((path = SkipElem(path, name)) != 0) {
		printf("name=%s\n", name);
retry:	
		if (!inode) {
			printf("NameSpace::Namei(%s): DONE 1\n", cpath);
			return -1;
		}
		if (nameiparent && *path == '\0') {
			// Stop one level early.
			//TODO: unlock inode after lookup and lock the next
			// follow superblock's root inode if parent is mount point
			if (typeid(*inode) == typeid(MPInode)) {
				if ((ret = inode->Lookup(name, &inode_next)) == -2) {
					MPInode* mpnode = dynamic_cast<MPInode*>(inode);
					sb = mpnode->GetSuperBlock();
					if ((uint64_t) sb != KERNEL_SUPERBLOCK) {
						inode = sb->GetRootInode();
					}
				}
			}
			goto done;
		}

		//FIXME: Lock (latch) inode before looking up/then unlock (i.e. do spider locking)
		//FIXME: Release inode when done 
		if ((ret = inode->Lookup(name, &inode_next)) < 0) {
			printf("NameSpace::Namei(%s): Lookup: %s: ret=%d\n", cpath, name, ret);
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
					inode = inode_next;
					goto retry;
				}
			} else {
				//printf("NameSpace::Namei(%s): not found\n", cpath);
				return ret;
			}	
		}
		printf("name=%s, inode_next=%p\n", name, inode_next);
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
NameSpace::Nameiparent(const char* path, char* name, Inode** inodep)
{
	int ret; 

	ret = Namex(path, true, name, inodep);
	return ret;
}


int
NameSpace::Namei(const char* path, Inode** inodep)
{
	int  ret; 
	char name[128];

	ret = Namex(path, false, name, inodep);
	return ret;
}


int
NameSpace::Link(const char *name, void *obj)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
}


int
NameSpace::Unlink(const char *name)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");
/*
	std::string        name_str;
	int                r;
	int                intret;

	name_str = std::string(name);

	intret = client_->call(RPC_NAME_UNLINK, principal_id_, name_str, r);
	if (intret) {
		return -intret;
	}

	return 0;
*/	
}
