#include <stdlib.h>
#include "pxfs/client/libfs.h"
#include "ubench/fs/pxfs.h"


int (*fs_open)(const char*, int flags) = libfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = libfs_open2;
int (*fs_unlink)(const char*) = libfs_unlink;
int (*fs_rename)(const char*, const char*) = libfs_rename;
int (*fs_close)(int fd) = libfs_close;
int (*fs_fsync)(int fd) = libfs_fsync;
int (*fs_sync)() = libfs_sync;
int (*fs_mkdir)(const char*, int mode) = libfs_mkdir;
ssize_t (*fs_write)(int fd, const void* buf, size_t count) = libfs_write;
ssize_t (*fs_read)(int fd, void* buf, size_t count) = libfs_read;
ssize_t (*fs_pwrite)(int fd, const void* buf, size_t count, off_t offset) = libfs_pwrite;
ssize_t (*fs_pread)(int fd, void* buf, size_t count, off_t offset) = libfs_pread;

RFile* (*fs_fopen)(const char*, int flags) = NULL;
ssize_t (*fs_fread)(RFile* fp, void* buf, size_t count) = NULL;
ssize_t (*fs_fpread)(RFile* fp, void* buf, size_t count, off_t offset) = NULL;
int (*fs_fclose)(RFile* fp) = NULL;


int 
Init(int debug_level, const char* xdst)
{
	int ret;

	libfs_init3(xdst, debug_level);
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
