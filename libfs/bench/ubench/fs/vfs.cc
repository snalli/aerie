#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "ubench/fs/vfs.h"

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

int vfs_mkdir(const char* pathname, int mode)
{
	return mkdir(pathname, mode);
}

int vfs_rename(const char *from, const char *to)
{
	return rename(from, to);
}

int (*fs_open)(const char*, int flags) = vfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = vfs_open2;
int (*fs_unlink)(const char*) = vfs_unlink;
int (*fs_rename)(const char*, const char*) = vfs_rename;
int (*fs_close)(int fd) = vfs_close;
int (*fs_fsync)(int fd) = fsync;
int (*fs_sync)() = vfs_sync;
int (*fs_mkdir)(const char*, int mode) = vfs_mkdir;
ssize_t (*fs_write)(int fd, const void* buf, size_t count) = write;
ssize_t (*fs_read)(int fd, void* buf, size_t count) = read;
ssize_t (*fs_pwrite)(int fd, const void* buf, size_t count, off_t offset) = pwrite;
ssize_t (*fs_pread)(int fd, void* buf, size_t count, off_t offset) = pread;

RFile* (*fs_fopen)(const char*, int flags) = NULL;
ssize_t (*fs_fread)(RFile* fp, void* buf, size_t count) = NULL;
ssize_t (*fs_fpread)(RFile* fp, void* buf, size_t count, off_t offset) = NULL;
int (*fs_fclose)(RFile* fp);



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
