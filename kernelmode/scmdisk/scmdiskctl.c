#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include "scmdisk.h"

static char* scmctrl_path = "/sys/kernel/scm-ctrl/";


void write_fs()
{
	int i;
	int fd;
	char buf[1024];

	for (i=0; i<1024; i++) {
		buf[i] = 'c';
	}
	fd = open("/mnt/scmfs/test", O_CREAT| O_RDWR, S_IRWXU);
	if (fd<0) {
		return;
	}
	while (1) {
		write(fd, buf, 1024);
		fsync(fd);
	}
}

int scmctrl_read(char* var)
{
	FILE* file;
	char  buf[512];
	int   val;

	strcpy(buf, scmctrl_path);
	strcat(buf, var);
	if (file = fopen(buf, "r")) {
		fscanf(file, "%d", &val);
		return val;
	}
	return -1;
}

uint64_t scmctrl_readu(char* var)
{
	FILE*    file;
	char     buf[512];
	uint64_t val;

	strcpy(buf, scmctrl_path);
	strcat(buf, var);
	if (file = fopen(buf, "r")) {
		fscanf(file, "%"SCNu64, &val);
		return val;
	}
	return -1;
}

void scmctrl_write(char* var, int val)
{
	FILE* file;
	char  buf[512];

	strcpy(buf, scmctrl_path);
	strcat(buf, var);
	if (file = fopen(buf, "w")) {
		fprintf(file, "%d", val);
	}
}


int
reset_statistics()
{
	scmctrl_write("statistics", 0);
	return 0;
}


int
print_statistics(FILE *fout)
{
	uint64_t bytes_written = scmctrl_readu("bytes_written");
	uint64_t blocks_written = scmctrl_readu("blocks_written");
	uint64_t total_write_latency = scmctrl_readu("total_write_latency");
	
	fprintf(fout, "bytes written            %" PRIu64 " (bytes)\n", bytes_written);
	fprintf(fout, "blocks written           %" PRIu64 " (blocks)\n", blocks_written);
	fprintf(fout, "total write latency      %" PRIu64 " (us)\n", total_write_latency/1000);
	fprintf(fout, "avg block write latency  %" PRIu64 " (ns)\n", (blocks_written > 0) ? total_write_latency/blocks_written : 0);
}



int
print_config(FILE *fout, int fd)
{
	int scm_latency;
	int scm_bw;
	int dram_bw;

	if ((scm_latency = scmctrl_read("SCM_LATENCY_NS")) < 0) {
		goto err;
	}
	if ((scm_bw = scmctrl_read("SCM_BANDWIDTH_MB")) < 0) {
		goto err;
	}
	if ((dram_bw = scmctrl_read("DRAM_BANDWIDTH_MB")) < 0) {
		goto err;
	}
	
	fprintf(fout, "SCMDISK CONFIGURATION\n");
	fprintf(fout, "==================================\n");
	fprintf(fout, "SCM latency           %d (ns)\n", scm_latency);
	fprintf(fout, "SCM bandwidth         %d (MB/s)\n", scm_bw);
	fprintf(fout, "DRAM bandwidth        %d (MB/s)\n", dram_bw);

	return 0;
err:
	fprintf(fout, "Cannot read SCM-disk configuration\n");
	return -1;
}

static
void usage(FILE *fout, char *name) 
{
	fprintf(fout, "usage:\n");
	fprintf(fout, "\n");
	fprintf(fout, "Options\n");
	fprintf(fout, "  %s  %s\n", "--crash          ", "Crash SCM-disk");
	fprintf(fout, "  %s  %s\n", "--reset          ", "Reset SCM-disk");
	fprintf(fout, "  %s  %s\n", "--print-config   ", "Print SCM-disk configuration");
	fprintf(fout, "  %s  %s\n", "--set-scm-latency", "Set SCM latency");
	fprintf(fout, "  %s  %s\n", "--set-scm-bw     ", "Set SCM bandwidth");
	fprintf(fout, "  %s  %s\n", "--set-dram-bw    ", "Set DRAM bandwidth");
	fprintf(fout, "  %s  %s\n", "--reset-stat     ", "Reset statistics");
	fprintf(fout, "  %s  %s\n", "--print-stat     ", "Print statistics");
	exit(1);
}

int
main(int argc, char **argv)
{
	int  fd;
	int  r;
	int  i;
	int  c;
	int  val;
	char *prog_name = argv[0];

	while (1) {
		static struct option long_options[] = {
			{"reset",  no_argument, 0, 'R'},
			{"crash",  no_argument, 0, 'C'},
			{"print-config", no_argument, 0, 'f'},
			{"set-scm-latency", required_argument, 0, 'l'},
			{"set-scm-bw", required_argument, 0, 'p'},
			{"set-dram-bw", required_argument, 0, 'd'},
			{"reset-stat", no_argument, 0, 'r'},
			{"print-stat", no_argument, 0, 's'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "RCfp:d:rs",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 'C':
				printf("Crash SCM disk.\n");
				r = ioctl(fd, 32000);
				break;
			case 'R':
				printf("Reset SCM disk.\n");
				r = ioctl(fd, 32001);
				break;
			case 'l':
				val = atoi(optarg);
				scmctrl_write("SCM_LATENCY_NS", val);
				break;
			case 'p':
				val = atoi(optarg);
				scmctrl_write("SCM_BANDWIDTH_MB", val);
				break;
			case 'd':
				val = atoi(optarg);
				scmctrl_write("DRAM_BANDWIDTH_MB", val);
				break;
			case 'f':
				if ((r=print_config(stderr, fd)) < 0) {
					goto err;
				}
				break;
			case 'w':
				write_fs();
				break;
			case 'r':
				reset_statistics();
				break;
			case 's':
				print_statistics(stderr);
				break;
			case '?':
				/* getopt_long already printed an error message. */
				usage(stderr, prog_name);
				break;
 
		}
	}

	close(fd);
	return 0;

err:
	return r;
}	
