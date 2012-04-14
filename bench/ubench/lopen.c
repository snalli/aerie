#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#define __USE_GNU
#define _GNU_SOURCE
#include <dlfcn.h>
#include "hrtime.h"
#include <valgrind/callgrind.h>

#include "/home/hvolos/workspace/muses/stamnos/libfs/src/pxfs/client/c_api.h"
#include "/home/hvolos/workspace/gperftools-2.0/src/gperftools/profiler.h"

typedef struct {
	void* fn;
	hrtime_t start;
} profile_t;

/*
static profile_t __thread profstack[1024];
static int __thread index = 0;

void __cyg_profile_func_enter (void *this_fn, void *call_site)
{
	profstack[index].fn = this_fn;
	profstack[index].start = gethrtime();
	index++;
	//printf("ENTER: %p\n", this_fn);
}

void __cyg_profile_func_exit (void *this_fn, void *call_site)
{
	Dl_info di;
	hrtime_t cycles;
	index--;
	//printf("EXIT: %p\n", this_fn);
	assert(profstack[index].fn == this_fn);
	cycles = gethrtime() - profstack[index].start;
	if (dladdr(this_fn, &di)) { 
		//printf(" %s (%s)", di.dli_sname ? di.dli_sname : "<unknown>", di.dli_fname); 
		printf(" %s: %llu\n", di.dli_sname ? di.dli_sname : "<unknown>", cycles); 
		//printf("%p: cycles=%llu\n", this_fn, cycles);
	} 
}
*/

int create_file(int iter, int nfiles)
{
	const char* str = "BEEF";
	char buf[512];
	char pathname[512];
	int fd;
	int i,j;
	hrtime_t hrstart;
	hrtime_t hrstop;
 	strcpy(buf, str);

	for (i=0; i<iter;i++) {
		for (j=0;j<nfiles;j++) {
 			sprintf(pathname, "/pxfs/test-%d.dat", j);
			hrstart = gethrtime();
			fd = libfs_open(pathname, O_CREAT| O_TRUNC);
			//libfs_fsync(fd);
			//hrstart = gethrtime();
			//libfs_write(fd, buf, 512);
			hrstop = gethrtime();
			printf("latency: %llu\n", hrstop - hrstart);
			libfs_close(fd);
		}
	}
	return 0;
}


int open_file(int iter, int nfiles)
{
	const char* str = "BEEF";
	char buf[512];
	char pathname[512];
	int fd;
	int i,j;
	hrtime_t hrstart;
	hrtime_t hrstop;
	printf("OPEN FILE: nfiles = %d\n", nfiles);

	for (i=0; i<iter;i++) {
		for (j=0;j<nfiles;j++) {
 			sprintf(pathname, "/pxfs/test-%d.dat", j);
			hrstart = gethrtime();
			//CALLGRIND_TOGGLE_COLLECT
			assert((fd = libfs_open(pathname, 0)) > 0);
			//CALLGRIND_TOGGLE_COLLECT
			//hrstart = gethrtime();
			//libfs_read(fd, buf, strlen(str)+1);
			//assert(strcmp(buf, str) == 0);
			hrstop = gethrtime();
			printf("latency: %llu\n", hrstop - hrstart);
			libfs_close(fd);
		}
	}
	return 0;
}



main(int argc, char **argv)
{
	int iter;
	int nfiles;
	char op;
	hrtime_t hrstart;
	hrtime_t hrstop;

	iter = atoi(argv[1]);
	nfiles = atoi(argv[2]);
	op = argv[3][0];
	printf("# iter    : %d\n", iter);
    
	libfs_init(argc, argv);
    libfs_mount("/tmp/stamnos_pool", "/pxfs", "mfs", 0);
    libfs_chdir("/pxfs");
	
	switch(op) {
		case 'c':
			create_file(iter, nfiles); 
			break;
		case 'o':
			open_file(iter, nfiles);
			break;
	}

	hrstart = gethrtime();
	libfs_sync();
	hrstop = gethrtime();
	printf("sync_latency: %llu\n", hrstop - hrstart);

	libfs_shutdown();
}
