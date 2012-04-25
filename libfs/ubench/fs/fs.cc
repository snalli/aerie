#include "ubench/main.h"
#include "ubench/fs/fs.h"

int 
RegisterUbench()
{
	ubench_table.push_back(UbenchDescriptor("fs_create", ubench_fs_create));
	ubench_table.push_back(UbenchDescriptor("fs_open", ubench_fs_open));
	ubench_table.push_back(UbenchDescriptor("fs_unlink", ubench_fs_unlink));
	ubench_table.push_back(UbenchDescriptor("fs_read", ubench_fs_read));
	return 0;
}
