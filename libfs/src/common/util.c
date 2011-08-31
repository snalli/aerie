#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include "util.h"

int 
create_backing_store(char *file, size_t size)
{
	int      fd;
	ssize_t  roundup_size;
	char     buf[1]; 
	
	fd = open(file, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		return R_FAILURE;
	}
	roundup_size = SIZEOF_PAGES(size);
	assert(lseek(fd, roundup_size, SEEK_SET) !=  (off_t) -1);
	write(fd, buf, 1);

	return fd;
}

int
check_backing_store(char *path, size_t size)
{
	int         rv = R_FAILURE;
	struct stat stat_buf;
	ssize_t     roundup_size;

	if (stat(path, &stat_buf)==0) {
		roundup_size = SIZEOF_PAGES(size);
		if (stat_buf.st_size > roundup_size) {
			rv = R_SUCCESS;
		} else {
			rv = R_FAILURE; 
		}
	}
	return rv;
}


