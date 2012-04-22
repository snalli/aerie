#include <stdlib.h>
#include "pxfs/client/libfs.h"
#include "ubench/fs/pxfs.h"

int 
RegisterUbench()
{
	ubench_table.push_back(UbenchDescriptor("fs_create", ubench_fs_create));
	ubench_table.push_back(UbenchDescriptor("fs_open", ubench_fs_open));
	ubench_table.push_back(UbenchDescriptor("fs_unlink", ubench_fs_unlink));
	return 0;
}


int (*fs_open)(const char*, int flags) = libfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = libfs_open2;
int (*fs_unlink)(const char*) = libfs_unlink;
int (*fs_close)(int fd) = libfs_close;
int (*fs_fsync)(int fd) = libfs_fsync;
int (*fs_sync)() = libfs_sync;
ssize_t (*fs_write)(int fd, const void* buf, size_t count) = libfs_write;
ssize_t (*fs_read)(int fd, void* buf, size_t count) = libfs_read;
ssize_t (*fs_pwrite)(int fd, const void* buf, size_t count, off_t offset) = libfs_pwrite;
ssize_t (*fs_pread)(int fd, void* buf, size_t count, off_t offset) = libfs_pread;

int 
Init(int debug_level, const char* xdst)
{
	int ret;

	if ((ret = Config::Init()) < 0) {
		return ret;
	}
	if ((ret = Debug::Init(debug_level, NULL)) < 0) {
		return ret;
	}
	libfs_init2(xdst);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
	libfs_chdir("/pxfs");
	return 0;
}


int
ShutDown()
{
	libfs_shutdown();
	return 0;
}
