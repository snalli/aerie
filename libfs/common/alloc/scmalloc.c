#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include "vistaheap.h"

static void *scmvheap;

#define PAGE_SIZE          4096
#define NUM_PAGES(size)    ((((size) % PAGE_SIZE) == 0? 0 : 1) + (size)/PAGE_SIZE)
#define SIZEOF_PAGES(size) (NUM_PAGES((size)) * PAGE_SIZE)


static int
file_create(char *file, size_t size)
{
    int      fd;
    ssize_t  roundup_size;
    char     buf[1];

    fd = open(file, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return -1;
    }
    roundup_size = SIZEOF_PAGES(size);
    assert(lseek(fd, roundup_size, SEEK_SET) !=  (off_t) -1);
    write(fd, buf, 1);

    return fd;
}

static int
file_exists(char *path, size_t size)
{
	int         rv = -1;
	struct stat stat_buf;
	ssize_t     roundup_size;

    if (stat(path, &stat_buf)==0) {
		roundup_size = SIZEOF_PAGES(size);
		if (stat_buf.st_size > roundup_size) {
			rv = 0;
		} else {
			rv = -1;
        }
    }

    return rv;
}


static void* heap_init(char* filename, size_t maxsize, void **root_ptr, size_t root_size)
{
	int        mmap_flags;
	vistaheap* vistaheapp;
	int        do_vista_init = 0;
	void*      vistaheap_base;
	void*      vistaheap_limit;
	int        vistaheap_fd;

	if (filename) {
		if (file_exists(filename, maxsize) < 0) {
			if ((vistaheap_fd = file_create(filename, maxsize)) < 0) {
				return NULL;
			}
			do_vista_init = 1;
		} else {
			vistaheap_fd = open(filename, O_RDWR);
		}	
		mmap_flags = MAP_SHARED;
	} else {
		do_vista_init = 0;
		vistaheap_fd = 0;
		mmap_flags = MAP_ANON | MAP_PRIVATE;
	}

	if ((vistaheap_base = mmap(0, maxsize, PROT_WRITE|PROT_READ, mmap_flags, vistaheap_fd, 0)) == (void *) -1) {
		close(vistaheap_fd);
		if (do_vista_init) {
			unlink(filename);
		}
		return NULL;
	}
	vistaheapp = (vistaheap *) ((uintptr_t) vistaheap_base);
	*root_ptr = (void *) ((uintptr_t) vistaheap_base + sizeof(vistaheap));
	vistaheap_base = (void *) ((uintptr_t) vistaheap_base + sizeof(vistaheap) + root_size);
	vistaheap_limit = (void *) ((uintptr_t) vistaheap_base + maxsize - root_size);
	if (do_vista_init) {
		vistaheap_init(vistaheapp, vistaheap_base, vistaheap_limit, vistaheapp, vistaheap_fd);
		memset(*root_ptr, 0, root_size);
	}
	printf("do_init = %d\n", do_vista_init);
	printf("root_ptr = %p\n", *root_ptr);
	printf("vistaheap_base = %p\n", vistaheap_base);
	return vistaheapp;
}


void
scmalloc_init(void** root_ptr, size_t root_size)
{
	scmvheap = heap_init("scmheap.dat", 1024*1024*512, root_ptr, root_size);
}


int
scmalloc(void **ptr, size_t size)
{
	int ret;

	*ptr = vistaheap_malloc(scmvheap, size);
	memset(*ptr, 0, size);
	if (*ptr) {
		return 0;
	}
	return -1;
}


int
scmrealloc(void **ptr, size_t old_size, size_t new_size)
{
	int ret;
	void *old_ptr = *ptr;
	void *new_ptr;

	new_ptr = vistaheap_malloc(scmvheap, new_size);
	if (!new_ptr) {
		return -1;
	}
	memset((void*) ((uintptr_t) new_ptr + old_size), 0, new_size-old_size);
	if (old_ptr) {
		memcpy(new_ptr, old_ptr, old_size);
		free(old_ptr, old_size);
	}	
	*ptr = new_ptr;
	return 0;
}


void
scmfree(void* ptr, size_t size)
{
	vistaheap_free(scmvheap, ptr, size);
}
