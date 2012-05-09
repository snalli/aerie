#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>



int 
Create(int debug_level, const char* xdst)
{
	int   ret;
	int   fd;
	char* buf = (char*) malloc(4096*1024);

	mkdir("/mnt/scmfs/dir",  S_IRUSR|S_IWUSR);
	mkdir("/mnt/scmfs/dir2", S_IRUSR|S_IWUSR);
	fd = open("/mnt/scmfs/dir/file1", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	assert(fd>0);
	write(fd, buf, 4096*1024);
	close(fd);
	fd = open("/mnt/scmfs/dir/file2", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	assert(fd>0);
	write(fd, buf, 4096*1024);
	close(fd);
	fd = open("/mnt/scmfs/file1", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	assert(fd>0);
	write(fd, buf, 4096*1024);
	close(fd);
	return 0;
}
