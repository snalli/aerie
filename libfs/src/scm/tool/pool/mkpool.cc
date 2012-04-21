#include <getopt.h>
#include <iostream>
#include "common/util.h"
#include "common/errno.h"
#include "scm/pool/pool.h"
#include "scm/tool/pool/main.h"
#include "pxfs/server/session.h"


static int 
usage(const char *name)
{
    fprintf(stderr, "usage: %s   %s\n", name                    , "");
}


int 
main_mkpool(int argc, char* argv[]) 
{
	extern char* optarg;
	extern int   optind;
	const char*  pathname = NULL;
	const char*  type = NULL;
	const char*  size_str = NULL;
	uint64_t     usize = 0;
	int          ret;
	char         c = 0;

	while ((c = getopt(argc, argv, "p:s:"))!=-1) {
		switch (c) {
     		case 'p':
				pathname = optarg;
				break;
			case 's':
				size_str = optarg;
				usize = StringToSize(size_str);
				break;
			default:
				break;
		}
	}

	std::cerr << "Create storage pool" << std::endl;

	if (pathname == NULL || 
	    usize == 0) 
	{
		return usage(prog_name);
	}

	std::cerr << "\tpathname = " << pathname << std::endl;
	std::cerr << "\tsize     = " << size_str << " (" << usize << " bytes)\n" << std::endl;

	if ((ret = StoragePool::Create(pathname, usize, 0)) < 0) {
		if (ret != -E_ERRNO) {
			std::cerr << ErrorToString(-ret) << std::endl; 
		} else {
			std::cerr << strerror(errno) << std::endl;
		}
		return ret;
	}

	return 0;
}


