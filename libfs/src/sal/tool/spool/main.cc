#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include "sal/pool/pool.h"
#include "common/util.h"
#include "common/errno.h"

static const char* prog_name = "spool";
static const char  __whitespaces[] = "                                                              ";
#define WHITESPACE(len) &__whitespaces[sizeof(__whitespaces) - (len) -1]

static int usage(const char *name);


int 
cmd_create(int argc, char* argv[], int optind) 
{
	if (optind + 2 > argc) {
		return usage(prog_name);
	}

	int         ret;
	const char* pathname = argv[optind];
	const char* size = argv[optind+1];
	uint64_t    usize = StringToSize(size);

	std::cerr << "Create storage pool" << std::endl;
	std::cerr << "\tpathname = " << pathname << std::endl;
	std::cerr << "\tsize     = " << size << " (" << usize << " bytes)\n" << std::endl;

	if ((ret = StoragePool::Create(pathname, usize)) < 0) {
		if (ret != -E_ERRNO) {
			std::cerr << ErrorToString(-ret) << std::endl; 
		} else {
			std::cerr << strerror(errno) << std::endl;
		}
		return ret;
	}
	
	return 0;
}


static int 
usage(const char *name)
{
    fprintf(stderr, "usage: %s   %s\n", name                    , "");
    fprintf(stderr, "       %s   %s\n", WHITESPACE(strlen(name)), "--create\tCreate pool $PATH $SIZE");
}

int
main(int argc, char* argv[])
{
	int          ret;
	int          c;
	extern char* optarg;
	extern int   optind;
	int (*command)(int, char* [], int) = NULL;

	while (1) {
		static struct option long_options[] = {
			{"create",  no_argument, 0, 'c'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "c",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 'c':
				command = cmd_create;
		}
	}

	if (!command) {
		return usage(prog_name);
	}
	if ((ret = command(argc, argv, optind)) == E_SUCCESS) {
		std::cerr << "SUCCESS!" << std::endl;
	}
	return ret;
}
