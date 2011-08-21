#include "client/c_api.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rpc/rpc.h"
#include "client/client_i.h"
#include "common/debug.h"

using namespace client;


int
FRONTAPI(init) (rpcc* rpc_client, int principal_id)
{
	return Client::Init(rpc_client, principal_id);
}


int 
FRONTAPI(open) (const char* pathname, int flags)
{
	int ret;

	if ((ret = Client::Open(pathname, flags, 0)) == KERNEL_VFS) {
		return open(pathname, flags);
	}
	return ret;
}


int FRONTAPI(close) (int fd)
{
	int ret;

	if ((ret = Client::Close(fd)) == KERNEL_VFS) {
		return close(fd);
	}
	return ret;
}


int FRONTAPI(dup) (int oldfd)
{
	int ret;

	if ((ret = Client::Duplicate(oldfd)) == KERNEL_VFS) {
		return dup(oldfd);
	}
	return ret;
}


int FRONTAPI(dup2) (int oldfd, int newfd)
{
	int ret;

	if ((ret = Client::Duplicate(oldfd, newfd)) == KERNEL_VFS) {
		return dup2(oldfd, newfd);
	}
	return ret;
}


int 
FRONTAPI(mount) (const char* source, 
                 const char* target, 
                 const char* fstype, 
                 uint32_t flags)
{
	return Client::Mount(source, target, fstype, flags);
}


int 
FRONTAPI(umount) (const char* target)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");	
}
 

int 
FRONTAPI(mkfs) (const char* target, 
                const char* fstype, 
                uint32_t flags)
{
	return Client::Mkfs(target, fstype, flags);
}


int FRONTAPI(mkdir) (const char* path, int mode)
{
	int ret;

	if ((ret = Client::Mkdir(path, mode)) == KERNEL_VFS) {
		return mkdir(path, mode);
	}
	return ret;
}


int FRONTAPI(rmdir) (const char* path)
{
	int ret;

	if ((ret = Client::Rmdir(path)) == KERNEL_VFS) {
		return rmdir(path);
	}
	return ret;
}
