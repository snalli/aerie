#include <stdio.h>
#include "pxfs/client/c_api.h"
#include <fcntl.h>
#include "ubench/main.h"

std::vector<UbenchDescriptor> ubench_table;

int main(int argc, char *argv[])
{
	const char *xdst = "10000";
	char dst[10];
	libfs_init3(xdst,0);
	int fd = libfs_open("/scratch/nvm/stamnos/pxfs/testfile",O_RDONLY);
	libfs_read(fd, dst, 6);
	libfs_close(fd);	
	libfs_shutdown();
	printf("\n Test line : %s\n",dst);
	return 0;
}
