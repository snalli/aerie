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
FRONTAPI(init) (int principal_id, char* xdst)
{
	return Client::Init(principal_id, xdst);
}


int
FRONTAPI(shutdown) ()
{
	return Client::Shutdown();
}


int 
FRONTAPI(open) (const char* pathname, int flags)
{
	int ret;

	if ((ret = Client::Open(pathname, flags, 0)) == -E_KVFS) {
		return open(pathname, flags);
	}
	return ret;
}


int FRONTAPI(close) (int fd)
{
	int ret;

	if ((ret = Client::Close(fd)) == -E_KVFS) {
		return close(fd);
	}
	return ret;
}


int FRONTAPI(dup) (int oldfd)
{
	int ret;

	if ((ret = Client::Duplicate(oldfd)) == -E_KVFS) {
		return dup(oldfd);
	}
	return ret;
}


int FRONTAPI(dup2) (int oldfd, int newfd)
{
	int ret;

	if ((ret = Client::Duplicate(oldfd, newfd)) == -E_KVFS) {
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


int 
FRONTAPI(mkdir) (const char* path, int mode)
{
	int ret;

	if ((ret = Client::MakeDirectory(path, mode)) == -E_KVFS) {
		return mkdir(path, mode);
	}
	return ret;
}


int 
FRONTAPI(rmdir) (const char* path)
{
	int ret;

	if ((ret = Client::Rmdir(path)) == -E_KVFS) {
		return rmdir(path);
	}
	return ret;
}


ssize_t 
FRONTAPI(write) (int fd, const void *buf, size_t count)
{
	int   ret;
	const char* src = reinterpret_cast<const char*>(buf);

	if ((ret = Client::Write(fd, src, count)) == -E_KVFS) {
		return write(fd, buf, count);
	}
	return ret;
}


ssize_t 
FRONTAPI(read) (int fd, void *buf, size_t count)
{
	int   ret;
	char* dst = reinterpret_cast<char*>(buf);

	if ((ret = Client::Read(fd, dst, count)) == -E_KVFS) {
		return read(fd, buf, count);
	}
	return ret;
}


off_t 
FRONTAPI(lseek) (int fd, off_t offset, int whence)
{
	int   ret;

	if ((ret = Client::Seek(fd, offset, whence)) == -E_KVFS) {
		return lseek(fd, offset, whence);
	}
	return ret;
}
