#include "types.h"
#include "defs.h"
#include "fcntl.h"


int main(int argc, char *argv[])
{
	int fd;
	int ret;
	char buf[512];
	libfs_init();

	switch(argv[1][0]) {
		case 'w':
			fd = libfs_open("/test", O_CREATE|O_RDWR, 0);
			if (fd < 0) {
				panic("libfs_open failed");
			}
			ret = libfs_write(fd, "test", 5);
			if (ret < 0) {
				panic("libfs_write failed");
			}
			
			ret = libfs_write(fd, "test", 5);
			if (ret < 0) {
				panic("libfs_write failed");
			}
			return;
		case 'r':
			fd = libfs_open("/test", O_RDWR, 0);
			if (fd < 0) {
				panic("libfs_open failed");
			}
			ret = libfs_read(fd, buf, 5);
			if (ret < 0) {
				panic("libfs_read failed");
			}
			printf("%s\n", buf);
			return;
		}
}
