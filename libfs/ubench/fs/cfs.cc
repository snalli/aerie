#include <stdlib.h>
#include "cfs/client/libfs.h"
#include "ubench/fs/cfs.h"

int (*fs_open)(const char*, int flags) = cfs_open;
int (*fs_open2)(const char*, int flags, mode_t mode) = cfs_open2;
int (*fs_unlink)(const char*) = cfs_unlink;
int (*fs_close)(int fd) = cfs_close;
int (*fs_fsync)(int fd) = cfs_fsync;
int (*fs_sync)() = cfs_sync;

int 
RegisterUbench()
{
	ubench_table.push_back(UbenchDescriptor("fs_create", ubench_fs_create));
	ubench_table.push_back(UbenchDescriptor("fs_open", ubench_fs_open));
	ubench_table.push_back(UbenchDescriptor("fs_unlink", ubench_fs_unlink));
	return 0;
}


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
