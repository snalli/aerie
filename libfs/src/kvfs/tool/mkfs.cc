#include <getopt.h>
#include <iostream>
#include "common/util.h"
#include "spa/pool/pool.h"
#include "kvfs/tool/main.h"
#include "kvfs/server/session.h"
#include "kvfs/server/fs.h"

static int 
usage(const char *name)
{
    fprintf(stderr, "usage: %s%s%s\n", name, " ", "create -p STORAGE_POOL_PATH -t TYPE -s SIZE");
	return -1;
}


int 
main_mkfs(int argc, char* argv[]) 
{
	extern char* optarg;
	const char*  pathname = NULL;
	const char*  size_str = NULL;
	uint64_t     usize = 0;
	char         c = 0;

	while ((c = getopt(argc, argv, "p:s:t:"))!=-1) {
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
	std::cerr << "Create filesystem" << std::endl;

	if (usize == 0 || 
	    pathname == NULL) 
	{
		return usage(prog_name);
	}

	std::cerr << "\tpathname = " << pathname << std::endl;
	std::cerr << "\tsize     = " << size_str << " (" << usize << " bytes)\n" << std::endl;
	
	return server::FileSystem::Make(pathname, usize, 1, 0);
}
