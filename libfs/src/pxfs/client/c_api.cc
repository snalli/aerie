#include "pxfs/client/c_api.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rpc/rpc.h"
#include "pxfs/client/client_i.h"
#include "bcs/main/common/cdebug.h"

using namespace client;


int
PXFS_FRONTAPI(init) (int argc, char* argv[])
{
	return Client::Init(argc, argv);
}


int
PXFS_FRONTAPI(init2) (const char* xdst)
{
	return Client::Init(xdst);
}

int
PXFS_FRONTAPI(init3) (const char* xdst, int debug_level)
{
	return Client::Init(xdst, debug_level);
}


int
PXFS_FRONTAPI(shutdown) ()
{
	return Client::Shutdown();
}


int 
PXFS_FRONTAPI(mount) (const char* source, 
                 const char* target, 
                 const char* fstype, 
                 uint32_t flags)
{
	return Client::Mount(source, target, fstype, flags);
}


int 
PXFS_FRONTAPI(umount) (const char* target)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");	
}
 

int 
PXFS_FRONTAPI(mkdir) (const char* path, int mode)
{
	int ret;

	if ((ret = Client::CreateDir(path, mode)) == -E_KVFS) {
		return mkdir(path, mode);
	}
	return ret;
}


int 
PXFS_FRONTAPI(rmdir) (const char* path)
{
	int ret;

	if ((ret = Client::DeleteDir(path)) == -E_KVFS) {
		return rmdir(path);
	}
	return ret;
}


int 
PXFS_FRONTAPI(rename) (const char* oldpath, const char* newpath)
{
	int ret;

	if ((ret = Client::Rename(oldpath, newpath)) == -E_KVFS) {
		return rename(oldpath, newpath);
	}
	return ret;
}


int 
PXFS_FRONTAPI(link) (const char* oldpath, const char* newpath)
{
	int ret;

	if ((ret = Client::Link(oldpath, newpath)) == -E_KVFS) {
		return link(oldpath, newpath);
	}
	return ret;
}


int 
PXFS_FRONTAPI(unlink) (const char* pathname)
{
	int ret;

	if ((ret = Client::Unlink(pathname)) == -E_KVFS) {
		return unlink(pathname);
	}
	return ret;
}


int 
PXFS_FRONTAPI(chdir) (const char* path)
{
	int ret;

	if ((ret = Client::SetCurWrkDir(path)) == -E_KVFS) {
		return chdir(path);
	}
	return ret;
}


char* 
PXFS_FRONTAPI(getcwd) (char* path, size_t size)
{
	int ret;

	if ((ret = Client::GetCurWrkDir(path, size)) == -E_KVFS) {
		return getcwd(path, size);
	}
	return (ret == E_SUCCESS) ? path: NULL;
}


int 
PXFS_FRONTAPI(open) (const char* pathname, int flags)
{
	int ret;

	if ((ret = Client::Open(pathname, flags, 0)) == -E_KVFS) {
		return open(pathname, flags);
	}
	return ret;
}


int 
PXFS_FRONTAPI(open2) (const char* pathname, int flags, mode_t mode)
{
	int ret;

	if ((ret = Client::Open(pathname, flags, 0)) == -E_KVFS) {
		return open(pathname, flags);
	}
	return ret;
}



int PXFS_FRONTAPI(close) (int fd)
{
	int ret;

	if ((ret = Client::Close(fd)) == -E_KVFS) {
		return close(fd);
	}
	return ret;
}


int PXFS_FRONTAPI(dup) (int oldfd)
{
	int ret;

	if ((ret = Client::Duplicate(oldfd)) == -E_KVFS) {
		return dup(oldfd);
	}
	return ret;
}


int PXFS_FRONTAPI(dup2) (int oldfd, int newfd)
{
	int ret;

	if ((ret = Client::Duplicate(oldfd, newfd)) == -E_KVFS) {
		return dup2(oldfd, newfd);
	}
	return ret;
}



ssize_t 
PXFS_FRONTAPI(write) (int fd, const void *buf, size_t count)
{
	int   ret;
	const char* src = reinterpret_cast<const char*>(buf);

	if ((ret = Client::Write(fd, src, count)) == -E_KVFS) {
		return write(fd, buf, count);
	}
	return ret;
}


ssize_t 
PXFS_FRONTAPI(read) (int fd, void *buf, size_t count)
{
	int   ret;
	char* dst = reinterpret_cast<char*>(buf);

	if ((ret = Client::Read(fd, dst, count)) == -E_KVFS) {
		return read(fd, buf, count);
	}
	return ret;
}


ssize_t 
PXFS_FRONTAPI(pwrite) (int fd, const void *buf, size_t count, off_t offset)
{
	int   ret;
	const char* src = reinterpret_cast<const char*>(buf);

	if ((ret = Client::WriteOffset(fd, src, count, offset)) == -E_KVFS) {
		return pwrite(fd, buf, count, offset);
	}
	return ret;
}


ssize_t 
PXFS_FRONTAPI(pread) (int fd, void *buf, size_t count, off_t offset)
{
	int   ret;
	char* dst = reinterpret_cast<char*>(buf);

	if ((ret = Client::ReadOffset(fd, dst, count, offset)) == -E_KVFS) {
		return pread(fd, buf, count, offset);
	}
	return ret;
}


off_t 
PXFS_FRONTAPI(lseek) (int fd, off_t offset, int whence)
{
	int   ret;

	if ((ret = Client::Seek(fd, offset, whence)) == -E_KVFS) {
		return lseek(fd, offset, whence);
	}
	return ret;
}


int 
PXFS_FRONTAPI(stat) (const char *path, struct stat *buf)
{
	int ret;
	if ((ret = Client::Stat(path, buf)) == -E_KVFS) {
		return stat(path, buf);
	}
	return ret;
}


int
PXFS_FRONTAPI(sync) ()
{
	return Client::Sync();
}


int
PXFS_FRONTAPI(fsync) (int fd)
{
	return Client::Sync(fd);
}
