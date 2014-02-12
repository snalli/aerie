#include <stdio.h>
#include "pxfs/client/c_api.h"
#include <fcntl.h>
#include "ubench/main.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

std::vector<UbenchDescriptor> ubench_table;

int main(int argc, char *argv[])
{
	const char *xdst = "10000";
/*	const char *name = "Sanketh";
	char dst[10];
	char filename[]="/pxfs/a/bb/cc/dd/ee/ff/gg/file.txt";
	char buf[128];
*/	char wait;
	int fd;
	libfs_init3(xdst,0);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
	char chr = 'a';
	char path[] = "/pxfs/0";
	for ( int i = 0; i < 26; ++i)
	{
		path[6] = chr;
		libfs_mkdir(path, 0);
		++chr;
	}

	libfs_sync();
/*
	int fd = libfs_open("/pxfs/a/gg/file.txt",O_RDWR);
	libfs_close(fd);


	libfs_mkdir("/pxfs/a",0);
	libfs_mkdir("/pxfs/a/bb",0);
	libfs_mkdir("/pxfs/a/bb/cc",0);
	libfs_mkdir("/pxfs/a/bb/cc/dd",0);
	libfs_mkdir("/pxfs/a/bb/cc/dd/ee",0);
	libfs_mkdir("/pxfs/a/bb/cc/dd/ee/ff",0);
	libfs_mkdir("/pxfs/a/bb/cc/dd/ee/ff/gg",0);

	getchar();
	fd = libfs_open(filename,O_CREAT|O_RDWR);
	libfs_write(fd, name, 8);
	libfs_sync();
	libfs_close(fd); 

	fd = libfs_open(filename,O_RDWR);
	libfs_read(fd, buf, 8);
	libfs_close(fd); 

	printf("\nBuf : %s", buf);
     */
 //     libfs_unlink(filename);
        libfs_shutdown();
	return 0;
}
