#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include "tool/perfremote/perfremote.h"
#include "fileset.h"
#include "src/common/hrtime.h"

//FIXME: pick_file may pick a directory. the problem is that the index includes both directory and file entries. we should fix the index to keep separate vectors for each entry type and depth so that we can pick anything we like at random

static void 
usage(const char* name)
{
	std::cout << "usage: " << name << " -p FILESET_ROOT_PATH [OPTION]" << std::endl;
	std::cout << std::endl;
	std::cout << "  -p FILESET_ROOT_PATH" << "\tfileset root path" << std::endl;
	std::cout << "  -f NUM_FILES        " << "\tnumber of files" << std::endl;
	std::cout << "  -i DIRWIDTH_INNER   " << "\tfiles/dirs per inner directory" << std::endl;
	std::cout << "  -l DIRWIDTH_LEAF    " << "\tfiles per leaf directory" << std::endl;
	std::cout << "  -n NUM_OPS          " << "\tnumber of operations" << std::endl;
	std::cout << "  -r NCOUNT           " << "\tread NCOUNT bytes" << std::endl;
	std::cout << "  -w NCOUNT           " << "\twrite NCOUNT bytes" << std::endl;
	std::cout << "  -s                  " << "\tstat (no open)" << std::endl;
	std::cout << "  -m                  " << "\tenable perf monitoring" << std::endl;
	exit(1);
}

int main(int argc, char** argv)
{
	const char* progname = "open";
	int         nfiles = 100000;
	int         nops = nfiles;
	int         ncount = 0;
	int         dirwidth_leaf = 10000;
	int         dirwidth_inner = 20;
	char*       fileset_root_path = NULL;
	bool        monitor = false;
	bool        do_stat = false;
	bool        do_read = false;
	bool        do_write = false;
	char        ch;

	opterr=0;
	while ((ch = getopt(argc, argv, "f:n:i:l:p:msr:w:"))!=-1) {
		switch (ch) {
			case 'f':
				nfiles = atoi(optarg);
				break;
			case 'i':
				dirwidth_inner = atoi(optarg);
				break;
			case 'l':
				dirwidth_leaf = atoi(optarg);
				break;
			case 'p':
				fileset_root_path = optarg;
				break;
			case 'n':
				nops = atoi(optarg);
				break;
			case 'm':
				monitor = true;
				break;
			case 's':
				do_stat = true;
				break;
			case 'r':
				do_read = true;
				ncount = atoi(optarg);
				break;
			case 'w':
				do_write = true;
				ncount = atoi(optarg);
				break;
			case '?':
				usage(progname);
			default:
				break;
		}
	}
	if (!fileset_root_path) {
		usage(progname);
	}

	FileSet fileset(fileset_root_path, nfiles, dirwidth_inner, dirwidth_leaf, FileSet::kExists);
	
	PERF_ATTACH(monitor)
	
	timeval start,stop,result;
	gettimeofday(&start,NULL);

	char dummy_buf[64*1024];

	hrtime_t      open_start;
	hrtime_t      open_stop;
	hrtime_t      open_cycles = 0;
	hrtime_t      read_start;
	hrtime_t      read_stop;
	hrtime_t      read_cycles = 0;
	hrtime_t      write_start;
	hrtime_t      write_stop;
	hrtime_t      write_cycles = 0;
	hrtime_t      close_start;
	hrtime_t      close_stop;
	hrtime_t      close_cycles = 0;
	
	for (int i=0; i < nops; i++) {
		struct stat buf;
		std::string path;
		FileSet::Entry* entry;
		fileset.pick_file(FileSet::kExists, &path, &entry);
		if (do_stat) {
			int ret = stat(path.c_str(), &buf);
			assert(ret == 0);
		} else {
			open_start = hrtime_cycles();
			int fd = open(path.c_str(), O_RDWR);
			open_stop = hrtime_cycles();
			assert(fd > 0);
			if (do_write) {
				write_start = hrtime_cycles();
				write(fd, dummy_buf, ncount);
				write_stop = hrtime_cycles();
			}
			if (do_read) {
				read_start = hrtime_cycles();
				read(fd, dummy_buf, ncount);
				read_stop = hrtime_cycles();
			}
			close_start = hrtime_cycles();
			close(fd);
			close_stop = hrtime_cycles();
		}
		open_cycles += open_stop - open_start;
		read_cycles += read_stop - read_start;
		write_cycles += write_stop - write_start;
		close_cycles += close_stop - close_start;
	}

	gettimeofday(&stop, NULL);
	timersub(&stop, &start,&result);
	std::cout << "elapsed time=" << result.tv_sec + result.tv_usec/1000000.0 
		  << " sec (" << (1000000*result.tv_sec + result.tv_usec) << " usec)" << std::endl;
	std::cout << "opem=" << open_cycles << " cycles" << std::endl;
	std::cout << "read=" << read_cycles << " cycles" << std::endl;
	std::cout << "write=" << write_cycles << " cycles" << std::endl;
	std::cout << "close=" << close_cycles << " cycles" << std::endl;
	PERF_DEATTACH(monitor)
	return 0;
}
