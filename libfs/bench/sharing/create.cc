#include <stdlib.h>
#include "pxfs/client/libfs.h"


int 
Create(int debug_level, const char* xdst)
{
	int   ret;
	int   fd;
	char* buf = (char*) malloc(1024*1024);

	libfs_init3(xdst, debug_level);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
	fd = libfs_open("/pxfs/file", O_CREAT|O_RDWR);
	assert(fd>0);
	libfs_write(fd, buf, 1024*1024);
	libfs_close(fd);
	libfs_sync();
	libfs_shutdown();
	return 0;
}
