#include "ubench/main.h"
#include "ubench/fs/fs.h"

int 
RegisterUbench()
{
	ubench_table.push_back(UbenchDescriptor("fs_create", ubench_fs_create));
	ubench_table.push_back(UbenchDescriptor("fs_delete", ubench_fs_delete));
	ubench_table.push_back(UbenchDescriptor("fs_append", ubench_fs_append));
	ubench_table.push_back(UbenchDescriptor("fs_open", ubench_fs_open));
	ubench_table.push_back(UbenchDescriptor("fs_unlink", ubench_fs_unlink));
	ubench_table.push_back(UbenchDescriptor("fs_read", ubench_fs_read));
	ubench_table.push_back(UbenchDescriptor("fs_fread", ubench_fs_fread));
	ubench_table.push_back(UbenchDescriptor("fs_seqread", ubench_fs_seqread));
	ubench_table.push_back(UbenchDescriptor("fs_randread", ubench_fs_randread));
	ubench_table.push_back(UbenchDescriptor("fs_seqwrite", ubench_fs_seqwrite));
	ubench_table.push_back(UbenchDescriptor("fs_randwrite", ubench_fs_randwrite));

	return 0;
}
