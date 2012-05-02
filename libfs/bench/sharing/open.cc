#include <stdlib.h>
#include "pxfs/client/libfs.h"


int 
Open(int debug_level, const char* xdst)
{
	int ret;
	int fd;
	char buf[4096];

	libfs_init3(xdst, debug_level);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);

	for (int i=0; i<4; i++) {
		fd = libfs_open("/pxfs/file", O_RDWR);
		assert(fd>0);
		libfs_write(fd, buf, 1024);
		libfs_close(fd);
		sleep(2);
	}
	libfs_shutdown();
	return 0;
}
