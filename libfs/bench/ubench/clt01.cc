#include <stdio.h>
#include "pxfs/client/c_api.h"
#include <fcntl.h>
#include "ubench/main.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

std::vector<UbenchDescriptor> ubench_table;

void* lock_cont(void *ptr)
{
	// Dummy variable
	int *x_ptr = (int *)ptr;
	if (*x_ptr == 1) 
		printf("hello");
	return libfs_lock_cont();
}
int main(int argc, char *argv[])
{
	const char *xdst = "10000";
	const char *name = "Sanketh";
	char dst[10];
	char filename[]="/pxfs/a/bb/cc/dd/ee/ff/gg/file.txt";
	char buf[128];
	char wait;
	int fd;
	int count = 0;
	libfs_init3(xdst,0);
	libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
	fd = libfs_open("/pxfs/file.txt", O_RDWR | O_CREAT);
	while(count < 4100) {
		libfs_write(fd, name, 8);
		count += 1;
	}
	libfs_close(fd);

	/*char chr = 'a';
	char path[] = "/pxfs/0";
	for ( int i = 0; i < 26; ++i)
	{
		path[6] = chr;
		libfs_mkdir(path, 0);
		++chr;
	}
*/
//	libfs_bfs();
/*
	libfs_mkdir("/pxfs/a",0);
	libfs_mkdir("/pxfs/a/bb",0);
	libfs_mkdir("/pxfs/a/cc",0);
	libfs_mkdir("/pxfs/a/dd",0);
	libfs_mkdir("/pxfs/a/ee",0);
	libfs_mkdir("/pxfs/a/ff",0);
	libfs_mkdir("/pxfs/a/gg",0);

	libfs_mkdir("/pxfs/b",0);
//	libfs_mkdir("/pxfs/b/bb",0);
//	libfs_mkdir("/pxfs/b/cc",0);
	libfs_mkdir("/pxfs/b/dd",0);
	libfs_mkdir("/pxfs/b/dd/ee",0);
//	libfs_mkdir("/pxfs/b/dd/eee",0);
//	libfs_mkdir("/pxfs/b/dd/eee/f",0);
//	libfs_mkdir("/pxfs/b/dd/eee/f/g",0);
//	libfs_mkdir("/pxfs/b/dd/eee/f/g/h",0);
//	libfs_mkdir("/pxfs/b/dd/eee/f/g/h/i",0);
	libfs_mkdir("/pxfs/b/dd/ee/ff",0);
//	libfs_mkdir("/pxfs/b/dd/ee/ff/gg",0);

	fd = libfs_open("/pxfs/b/dd/ee/ff/file.txt",O_CREAT|O_RDWR);
	//libfs_write(fd, name, 8);
	libfs_close(fd); 

	libfs_sync();

	//fd = libfs_open("/pxfs/b/dd/ee/ff/file.txt",O_RDWR);
	//libfs_close(fd); 
	getchar();
	fd = libfs_open(filename,O_CREAT|O_RDWR);
	libfs_write(fd, name, 8);
	libfs_sync();
	libfs_close(fd); 

	fd = libfs_open(filename,O_RDWR);
	libfs_read(fd, buf, 8);
	libfs_close(fd); 

	printf("\nBuf : %s", buf);
     
 //     libfs_unlink(filename);
	libfs_lock_cont();
//	pthread_t t_1;
//	int i;
//	pthread_create(&t_1, NULL, lock_cont, (void *)&i);
///	void *res;
//	pthread_join(t_1, &res);
*/
        libfs_shutdown();
	return 0;
}
