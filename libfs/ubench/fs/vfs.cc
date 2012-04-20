#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ubench/fs/vfs.h"

int 
RegisterUbench()
{
	ubench_table.push_back(UbenchDescriptor("fs_create", ubench_fs_create));
	ubench_table.push_back(UbenchDescriptor("fs_open", ubench_fs_open));
	ubench_table.push_back(UbenchDescriptor("fs_unlink", ubench_fs_unlink));
	return 0;
}


int vfs_open(const char* path, int flags)
{
	return open(path, flags);
}

int vfs_open2(const char* path, int flags, mode_t mode)
{
	return open(path, flags, mode);
}

int vfs_unlink(const char* path)
{
	return unlink(path);
}

int vfs_close(int fd)
{
	return close(fd);
}

int vfs_sync()
{
	sync();
	return 0;
}


int (*fs_open)(const char*, int flags) = vfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = vfs_open2;
int (*fs_unlink)(const char*) = vfs_unlink;
int (*fs_close)(int fd) = vfs_close;
int (*fs_fsync)(int fd) = fsync;
int (*fs_sync)() = vfs_sync;

int 
Init(int debug_level, const char* xdst)
{
	return 0;
}


int
ShutDown()
{
	return 0;
}
