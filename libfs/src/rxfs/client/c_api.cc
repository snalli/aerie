#include "rxfs/client/c_api.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rxfs/client/client.h"
#include "rxfs/client/file.h"
#include "bcs/main/common/cdebug.h"

using namespace rxfs::client;

struct RFile;

int
RXFS_FRONTAPI(init) (int argc, char* argv[])
{
	return Client::Init(argc, argv);
}


int
RXFS_FRONTAPI(init2) (const char* xdst)
{
	return Client::Init(xdst);
}

int
RXFS_FRONTAPI(init3) (const char* xdst, int debug_level)
{
	return Client::Init(xdst, debug_level);
}


int
RXFS_FRONTAPI(shutdown) ()
{
	return Client::Shutdown();
}


int 
RXFS_FRONTAPI(mount) (const char* source, const char* target, 
                 const char* fstype, uint32_t flags)
{
	return Client::Mount(source, target, fstype, flags);
}


int 
RXFS_FRONTAPI(umount) (const char* target)
{
	dbg_log (DBG_CRITICAL, "Unimplemented functionality\n");	
}
 

int 
RXFS_FRONTAPI(mkdir) (const char* path, int mode)
{
	return Client::CreateDir(path, mode);
}


int 
RXFS_FRONTAPI(rmdir) (const char* path)
{
	return Client::DeleteDir(path);
}


int 
RXFS_FRONTAPI(rename) (const char* oldpath, const char* newpath)
{
	return Client::Rename(oldpath, newpath);
}


int 
RXFS_FRONTAPI(link) (const char* oldpath, const char* newpath)
{
	return Client::Link(oldpath, newpath);
}


int 
RXFS_FRONTAPI(unlink) (const char* pathname)
{
	return Client::Unlink(pathname);
}


int 
RXFS_FRONTAPI(chdir) (const char* path)
{
	int ret;

	if ((ret = Client::SetCurWrkDir(path)) == -E_KVFS) {
		return chdir(path);
	}
	return ret;
}


char* 
RXFS_FRONTAPI(getcwd) (char* path, size_t size)
{
	int ret = Client::GetCurWrkDir(path, size);
	return (ret == E_SUCCESS) ? path: NULL;
}


int 
RXFS_FRONTAPI(open) (const char* pathname, int flags)
{
	return  Client::Open(pathname, flags, 0);
}


int 
RXFS_FRONTAPI(open2) (const char* pathname, int flags, mode_t mode)
{
	return Client::Open(pathname, flags, 0);
}


int RXFS_FRONTAPI(close) (int fd)
{
	return Client::Close(fd);
}


int RXFS_FRONTAPI(dup) (int oldfd)
{
	return Client::Duplicate(oldfd);
}


int RXFS_FRONTAPI(dup2) (int oldfd, int newfd)
{
	return Client::Duplicate(oldfd, newfd);
}


ssize_t 
RXFS_FRONTAPI(write) (int fd, const void *buf, size_t count)
{
	const char* src = reinterpret_cast<const char*>(buf);

	return Client::Write(fd, src, count);
}


ssize_t 
RXFS_FRONTAPI(read) (int fd, void *buf, size_t count)
{
	char* dst = reinterpret_cast<char*>(buf);

	return Client::Read(fd, dst, count);
}


ssize_t 
RXFS_FRONTAPI(pwrite) (int fd, const void *buf, size_t count, off_t offset)
{
	const char* src = reinterpret_cast<const char*>(buf);

	return Client::WriteOffset(fd, src, count, offset);
}


ssize_t 
RXFS_FRONTAPI(pread) (int fd, void *buf, size_t count, off_t offset)
{
	char* dst = reinterpret_cast<char*>(buf);

	return Client::ReadOffset(fd, dst, count, offset);
}


off_t 
RXFS_FRONTAPI(lseek) (int fd, off_t offset, int whence)
{
	return Client::Seek(fd, offset, whence);
}


int 
RXFS_FRONTAPI(stat) (const char *path, struct stat *buf)
{
	return Client::Stat(path, buf);
}


int
RXFS_FRONTAPI(sync) ()
{
	return Client::Sync();
}


int
RXFS_FRONTAPI(fsync) (int fd)
{
	return Client::Sync(fd);
}


RFile*  
RXFS_FRONTAPI(fopen) (const char* pathname, int flags)
{
	int   ret;
	File* file;
	if ((ret = Client::Open(pathname, flags, 0, &file)) != E_SUCCESS) {
		return NULL;
	}
	return reinterpret_cast<RFile*>(file);
}


ssize_t 
RXFS_FRONTAPI(fread) (RFile* rfile, void *buf, size_t count)
{
	int   ret;
	char* dst = reinterpret_cast<char*>(buf);
	File* file = reinterpret_cast<File*>(rfile);
	return ret = Client::Read(file, dst, count);
}


ssize_t 
RXFS_FRONTAPI(fpread) (RFile* rfile, void *buf, size_t count, off_t offset)
{
	char* dst = reinterpret_cast<char*>(buf);
	File* file = reinterpret_cast<File*>(rfile);

	return Client::ReadOffset(file, dst, count, offset);
}


int
RXFS_FRONTAPI(fclose) (RFile* rfile)
{
	File* file = reinterpret_cast<File*>(rfile);
	return Client::Close(file);
}
