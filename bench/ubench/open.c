#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "hrtime.h"


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
 			sprintf(pathname, "/mnt/scmfs/test-%d.dat", j);
			hrstart = gethrtime();
			assert((fd = open(pathname, O_CREAT| O_TRUNC| O_RDWR, S_IRWXU)) > 0) ;
			//fsync(fd);
			//hrstart = gethrtime();
			//write(fd, buf, 512);
			hrstop = gethrtime();
			printf("latency: %llu\n", hrstop - hrstart);
			close(fd);
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
 			sprintf(pathname, "/mnt/scmfs/test-%d.dat", j);
			hrstart = gethrtime();
			assert((fd = open(pathname, O_RDWR)) > 0);
			//hrstart = gethrtime();
			//read(fd, buf, strlen(str)+1);
			//assert(strcmp(buf, str) == 0);
			hrstop = gethrtime();
			printf("latency: %llu\n", hrstop - hrstart);
			close(fd);
		}
	}
	return 0;
}


main(int argc, char **argv)
{
	char buf[512];
	int fd;
	int i,j;
	int iter;
	int nfiles;
	char op;
	struct timeval start, end;
	suseconds_t usecs;
	hrtime_t hrstart;
	hrtime_t hrstop;

	iter = atoi(argv[1]);
	nfiles = atoi(argv[2]);
	op = argv[3][0];
	printf("# iter    : %d\n", iter);

	switch(op) {
		case 'c':
			create_file(iter, nfiles); 
			break;
		case 'o':
			open_file(iter, nfiles);
			break;
	}
}
