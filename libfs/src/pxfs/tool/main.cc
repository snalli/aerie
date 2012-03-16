#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include "common/util.h"
#include "common/errno.h"
#include "pxfs/tool/main.h"
#include "pxfs/server/fs.h"
#include "ssa/main/server/ssa.h"
#include "pxfs/mfs/server/mfs.h"

const char* prog_name = "pxfs";

::server::FileSystem*          fs;
::ssa::server::StorageSystem*  storage_system;

struct Command {
	const char* name;
	const char* description;
	int (*command)(int, char* []);
};


static Command command_table[] = {
	{ "create", "Create filesystem", main_mkfs},
	{ NULL, NULL, NULL} // indicates end of table
};


static int 
usage(const char *prog_name)
{
    fprintf(stderr, "usage: %s   %s\n", prog_name                    , "");
	for (int i=0; command_table[i].name != NULL; i++) {
		const char* name = command_table[i].name;
		const char* description = command_table[i].description;
	    fprintf(stderr, "       %s   %s\t%s\n", WHITESPACE(strlen(prog_name)), name, description);
	}
}


int Init()
{
	storage_system = new ::ssa::server::StorageSystem(NULL);
	storage_system->Init();

	fs = new ::server::FileSystem(NULL, storage_system);
	fs->Init();
}


int
main(int argc, char* argv[])
{
	int          ret;
	int (*command)(int, char* []) = NULL;

	for (int i=0; command_table[i].name != NULL; i++) {
		const char* name = command_table[i].name;
		command = command_table[i].command;
	}

	if (!command) {
		return usage(prog_name);
	}

	if ((ret = Init()) < 0) {
		return ret;
	}
	
	if ((ret = command(argc, argv)) == E_SUCCESS) {
		std::cerr << "SUCCESS!" << std::endl;
	} else {
		std::cerr << "FAILURE" << std::endl;
	}
	return ret;
}
