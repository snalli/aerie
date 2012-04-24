#include "cfs/client/c_api.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rpc/rpc.h"
#include "cfs/client/client_i.h"
#include "bcs/main/common/cdebug.h"

using namespace client;


int
FRONTAPI(init) (int argc, char* argv[])
{
	return Client::Init(argc, argv);
}


int
FRONTAPI(init2) (const char* xdst)
{
	return Client::Init(xdst);
}


int
FRONTAPI(shutdown) ()
{
	return Client::Shutdown();
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
FRONTAPI(mkdir) (const char* path, int mode)
{
	int ret;

	if ((ret = Client::CreateDir(path, mode)) == -E_KVFS) {
		return mkdir(path, mode);
	}
	return ret;
}


int 
FRONTAPI(rmdir) (const char* path)
{
	return Client::DeleteDir(path);
}


int 
FRONTAPI(rename) (const char* oldpath, const char* newpath)
{
	return Client::Rename(oldpath, newpath);
}


int 
FRONTAPI(link) (const char* oldpath, const char* newpath)
{
	return Client::Link(oldpath, newpath);
}


int 
FRONTAPI(unlink) (const char* pathname)
{
	return Client::Unlink(pathname);
}


int 
FRONTAPI(chdir) (const char* path)
{
	return Client::SetCurWrkDir(path);
}


char* 
FRONTAPI(getcwd) (char* buf, size_t size)
{
	int ret;

	if ((ret = Client::GetCurWrkDir(buf, size)) < 0) {
		return NULL;
	}
	return buf;
}


int 
FRONTAPI(open) (const char* pathname, int flags)
{
	return Client::Open(pathname, flags, 0);
}


int 
FRONTAPI(open2) (const char* pathname, int flags, mode_t mode)
{
	return Client::Open(pathname, flags, 0);
}


int FRONTAPI(close) (int fd)
{
	return Client::Close(fd);
}


int FRONTAPI(dup) (int oldfd)
{
	int ret;

	return Client::Duplicate(oldfd);
}


int FRONTAPI(dup2) (int oldfd, int newfd)
{
	int ret;

	return Client::Duplicate(oldfd, newfd);
}


ssize_t 
FRONTAPI(write) (int fd, const void *buf, size_t count)
{
	int   ret;
	const char* src = reinterpret_cast<const char*>(buf);

	return Client::Write(fd, src, count);
}


ssize_t 
FRONTAPI(read) (int fd, void *buf, size_t count)
{
	int   ret;
	char* dst = reinterpret_cast<char*>(buf);

	return Client::Read(fd, dst, count);
}


ssize_t 
FRONTAPI(pwrite) (int fd, const void *buf, size_t count, off_t offset)
{
	int   ret;
	const char* src = reinterpret_cast<const char*>(buf);

	return Client::WriteOffset(fd, src, count, offset);
}


ssize_t 
FRONTAPI(pread) (int fd, void *buf, size_t count, off_t offset)
{
	int   ret;
	char* dst = reinterpret_cast<char*>(buf);

	return Client::ReadOffset(fd, dst, count, offset);
}


off_t 
FRONTAPI(lseek) (int fd, off_t offset, int whence)
{
	int   ret;

	return Client::Seek(fd, offset, whence);
}


int 
FRONTAPI(stat) (const char *path, struct stat *buf)
{
	int ret;
	if ((ret = Client::Stat(path, buf)) == -E_KVFS) {
		return stat(path, buf);
	}
	return ret;
}


int
FRONTAPI(sync) ()
{
	return Client::Sync();
}


int
FRONTAPI(fsync) (int fd)
{
	return Client::Sync(fd);
}
