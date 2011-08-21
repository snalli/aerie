#include "server/namespace.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

const int NAMESIZ = 128;

NameSpace::NameSpace()
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
}


// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char*
SkipElem(char *path, char *name)
{
	char* s;
	int   len;

	while(*path == '/') {
		path++;
	}	
	if(*path == 0) {
		return 0;
	}	
	s = path;
	while(*path != '/' && *path != 0) {
		path++;
	}	
	len = path - s;
	if(len >= NAMESIZ) {
		memmove(name, s, NAMESIZ);
	} else {
		memmove(name, s, len);
		name[len] = 0;
	}
	while(*path == '/') {
		path++;
	}	
	return path;
}


void* 
initfile(char* path, int flags, size_t hdrlen, size_t reclen, size_t reccnt)
{
	int                 i;
	int                 fd;
	int                 ret;
	size_t              length;
	pthread_mutexattr_t psharedm_attr;
	char*               tmpfilename;
	void*               ptr;

	length = reclen*reccnt + hdrlen;
	
	// always remove the file
	unlink(path);

	if ((fd = open(path, O_RDWR)) < 0) {
		/* Atomically create and initialize the file */
		do {
			tmpfilename = tempnam("/tmp", NULL);
			fd = open(tmpfilename, O_RDWR | O_CREAT | O_EXCL, 0666);
		} while (fd < 0 && errno == EEXIST);
		if (fd < 0) {
			return NULL;
		}
		ret = ftruncate(fd, length);
		if (ret < 0) {
			close (fd);
			return NULL;
		}
		ptr = mmap(NULL, length,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				fd, 0);
    	close (fd);

		if ((ret = link(tmpfilename, path)) < 0) {
			if (errno == EEXIST) {
			}
		}
		/* We unmap and remap below to ensure that the mapped file appears 
		 * in the address space named as 'path'. This simplifies locating
		 * where the file is mapped in our address space and hence makes 
		 * debugging easier.
		 */ 
		munmap((void *) ptr, length);
		unlink(tmpfilename);
		fd = open(path, O_RDWR);
	}

	/* we map  file */
	ptr = mmap(NULL, length,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	close(fd);

	return ptr;
}




int NameSpace::Init()
{
	// map the pseudo inodes file
	// if it doesn't exist, create it
	initfile("namespace.map", 0, 0, 0, 0);
}




int NameSpace::Mount(const char* name, void* superblock )
{
	printf("%s\n", name);
/*
	char  name[128];

	while((path = skipelem(path, name)) != 0){
	//	ilock(ip);
	}	
*/
}


int NameSpace::Unmount(const char* name)
{
	return 0;
}
