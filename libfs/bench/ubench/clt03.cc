#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
//std::vector<UbenchDescriptor> ubench_table;

int main(int argc, char *argv[])
{
	char chr = 'a';
	char path[] = "/home/sanketh/ext4/0";
	mkdir("/home/sanketh/ext4", 777);
	int l = strlen(path) - 1;
	for ( int i = 0; i < 26; ++i)
	{
		path[l] = chr;
		mkdir(path, 777);
		++chr;
	}

	sync();
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
//        libfs_shutdown();
	return 0;
}
