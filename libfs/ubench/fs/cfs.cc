#include <stdlib.h>
#include "cfs/client/libfs.h"
#include "ubench/fs/cfs.h"

int (*fs_open)(const char*, int flags) = cfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = cfs_open2;
int (*fs_unlink)(const char*) = cfs_unlink;
int (*fs_close)(int fd) = cfs_close;
int (*fs_fsync)(int fd) = cfs_fsync;
int (*fs_sync)() = cfs_sync;
ssize_t (*fs_write)(int fd, const void* buf, size_t count) = cfs_write;
ssize_t (*fs_read)(int fd, void* buf, size_t count) = cfs_read;
ssize_t (*fs_pwrite)(int fd, const void* buf, size_t count, off_t offset) = cfs_pwrite;
ssize_t (*fs_pread)(int fd, void* buf, size_t count, off_t offset) = cfs_pread;


int 
Init(int debug_level, const char* xdst)
{
	cfs_init2(xdst);
	cfs_mount("/tmp/stamnos_pool", "/", "cfs", 0);
	cfs_mkdir("/pxfs/", 0);
	return 0;
}


int
ShutDown()
{
	cfs_shutdown();
	return 0;
}
