#include <stdlib.h>
#include "cfs/client/libfs.h"
#include "ubench/fs/cfs.h"

int dummy(const char* /*f*/, const char* /*t*/)
{
	return 0;
}

int (*fs_open)(const char*, int flags) = cfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = cfs_open2;
int (*fs_unlink)(const char*) = cfs_unlink;
int (*fs_rename)(const char*, const char*) = dummy;
int (*fs_close)(int fd) = cfs_close;
int (*fs_fsync)(int fd) = cfs_fsync;
int (*fs_sync)() = cfs_sync;
int (*fs_mkdir)(const char*, int mode) = cfs_mkdir;
ssize_t (*fs_write)(int fd, const void* buf, size_t count) = cfs_write;
ssize_t (*fs_read)(int fd, void* buf, size_t count) = cfs_read;
ssize_t (*fs_pwrite)(int fd, const void* buf, size_t count, off_t offset) = cfs_pwrite;
ssize_t (*fs_pread)(int fd, void* buf, size_t count, off_t offset) = cfs_pread;

RFile* (*fs_fopen)(const char*, int flags) = NULL;
ssize_t (*fs_fread)(RFile* fp, void* buf, size_t count) = NULL;
ssize_t (*fs_fpread)(RFile* fp, void* buf, size_t count, off_t offset) = NULL;
int (*fs_fclose)(RFile* fp);



int
Init(int /*debug_level*/, const char* xdst)
{
	int ret;
	if ((ret = cfs_init2(xdst)) < 0) {
		fprintf(stderr, "cfs_init2 failed: %d\n", ret);
		return ret;
	}
	if ((ret = cfs_mount("/tmp/stamnos_pool", "/", "cfs", 0)) < 0) {
		fprintf(stderr, "cfs_mount failed: %d\n", ret);
		return ret;
	}
	if ((ret = cfs_mkdir("/pxfs/", 0)) < 0 && ret != -17 /* EEXIST */) {
		fprintf(stderr, "cfs_mkdir failed: %d\n", ret);
		return ret;
	}
	return 0;
}


int
ShutDown()
{
	cfs_shutdown();
	return 0;
}
