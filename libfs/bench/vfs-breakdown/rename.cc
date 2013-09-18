#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "tool/perfremote/perfremote.h"
#include "fileset.h"

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
	std::cout << "  -m                  " << "\tenable perf monitoring" << std::endl;
	exit(1);
}

int main(int argc, char** argv)
{
	const char* progname = "unlink";
	int         nfiles = 100000;
	int         nops = nfiles;
	int         dirwidth_leaf = 10000;
	int         dirwidth_inner = 20;
	char*       fileset_root_path = NULL;
	bool        monitor = false;
	bool        execute_stat = false;
	char        ch;

	opterr=0;
	while ((ch = getopt(argc, argv, "f:n:i:l:p:ms"))!=-1) {
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

	for (int i=0; i < nops; i++) {
		std::string path;
		std::string newpath;
		FileSet::Entry* entry;
		fileset.pick_file(FileSet::kExists, &path, &entry);
		newpath = path + ".renamed";
		int ret = rename(path.c_str(), newpath.c_str());
		assert(ret == 0);
		fileset.set_state(entry, FileSet::kNotExists);
	}

	gettimeofday(&stop, NULL);
	timersub(&stop, &start,&result);
	std::cout << "elapsed time=" << result.tv_sec + result.tv_usec/1000000.0 
		  << " sec (" << (1000000*result.tv_sec + result.tv_usec) << " usec)" << std::endl;

	PERF_DEATTACH(monitor)
	return 0;
}
