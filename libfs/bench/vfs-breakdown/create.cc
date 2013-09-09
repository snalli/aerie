#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include "tool/perfremote/perfremote.h"
#include "fileset.h"

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
	std::cout << "  -s FILE_SIZE        " << "\tfile size (in bytes)" << std::endl;
	std::cout << "  -n NUM_OPS          " << "\tnumber of random operations (optional, if not given then NUM_FILES are created)" << std::endl;
	std::cout << "  -m                  " << "\tenable perf monitoring" << std::endl;
	exit(1);
}


int main(int argc, char** argv)
{
	const char* progname = "create";
	int         nfiles = 100000;
	int         nops;
	int         dirwidth_leaf = 10000;
	int         dirwidth_inner = 20;
	int         file_size = 0;
	char*       fileset_root_path = NULL;
	bool        monitor = false;
	bool        random = false;
	char        ch;

	opterr=0;
	while ((ch = getopt(argc, argv, "f:i:l:p:s:mn:r"))!=-1) {
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
			case 's':
				file_size = atoi(optarg);
				break;
			case 'm':
				monitor = true;
				break;
			case 'n':
				nops = atoi(optarg);
				random = true;
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


	FileSet fileset(fileset_root_path, nfiles, dirwidth_inner, dirwidth_leaf, FileSet::kNotExists);
	

	PERF_ATTACH(monitor)
	
	timeval start,stop,result;
	gettimeofday(&start,NULL);

	if (!random) {
		fileset.create_files(file_size);
	} else {
		for (int i=0; i < nops; i++) {
			std::string path;
			FileSet::Entry* entry;
			fileset.pick_file(FileSet::kNotExists, &path, &entry);
			int fd = open(path.c_str(), O_CREAT | O_WRONLY, S_IRWXU);
			assert(fd > 0);
			close(fd);
			fileset.set_state(entry, FileSet::kExists);
		}
	}

	gettimeofday(&stop, NULL);
	timersub(&stop, &start,&result);
	std::cout << "elapsed time=" << result.tv_sec + result.tv_usec/1000000.0 
		  << " sec (" << (1000000*result.tv_sec + result.tv_usec) << " usec)" << std::endl;

	PERF_DEATTACH(monitor)
	return 0;
}
