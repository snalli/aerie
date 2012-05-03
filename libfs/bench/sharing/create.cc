#include <stdlib.h>
#include "pxfs/client/libfs.h"


int 
Create(int debug_level, const char* xdst)
{
	int   ret;
	int   fd;
	char* buf = (char*) malloc(4096*1024);

	libfs_init3(xdst, debug_level);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
	libfs_mkdir("/pxfs/dir", 0);
	libfs_mkdir("/pxfs/dir2", 0);
	fd = libfs_open("/pxfs/dir/file1", O_CREAT|O_RDWR);
	assert(fd>0);
	libfs_write(fd, buf, 4096*1024);
	libfs_close(fd);
	fd = libfs_open("/pxfs/dir/file2", O_CREAT|O_RDWR);
	assert(fd>0);
	libfs_write(fd, buf, 4096*1024);
	libfs_close(fd);
	fd = libfs_open("/pxfs/dir2/file1", O_CREAT|O_RDWR);
	assert(fd>0);
	libfs_write(fd, buf, 4096*1024);
	libfs_close(fd);
	libfs_sync();
	libfs_shutdown();
	return 0;
}
