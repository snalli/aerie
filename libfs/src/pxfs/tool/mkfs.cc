#include <getopt.h>
#include <iostream>
#include "common/util.h"
#include "spa/pool/pool.h"
#include "pxfs/tool/main.h"
#include "server/session.h"

static int 
usage(const char *name)
{
    fprintf(stderr, "usage: %s%s%s\n", name, " ", "create -p STORAGE_POOL_PATH -t TYPE -s SIZE");
}


int 
main_mkfs(int argc, char* argv[]) 
{
	extern char* optarg;
	extern int   optind;
	const char*  pathname = NULL;
	const char*  type = NULL;
	const char*  size_str = NULL;
	uint64_t     usize = 0;
	int          ret;
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
			case 't': 
				type = optarg;
				break;
			default:
				break;
		}
	}
	std::cerr << "Create filesystem" << std::endl;

	if (usize == 0 || 
	    type == NULL || 
	    pathname == NULL) 
	{
		return usage(prog_name);
	}

	std::cerr << "\tpathname = " << pathname << std::endl;
	std::cerr << "\ttype     = " << type << std::endl;
	std::cerr << "\tsize     = " << size_str << " (" << usize << " bytes)\n" << std::endl;
	
	return fsmgr->CreateFileSystem(pathname, type, usize, 1, 0);
}


