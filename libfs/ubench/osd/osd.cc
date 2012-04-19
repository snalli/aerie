#include "ubench/osd/osd.h"

using namespace client;

const char*                 poolpath = "/tmp/stamnos_pool";


int 
RegisterUbench()
{
	ubench_table.push_back(UbenchDescriptor("create", ubench_create));
	ubench_table.push_back(UbenchDescriptor("open", ubench_open));
	ubench_table.push_back(UbenchDescriptor("lock", ubench_lock));
	ubench_table.push_back(UbenchDescriptor("hlock", ubench_hlock));
	ubench_table.push_back(UbenchDescriptor("hlock_create", ubench_hlock_create));
	ubench_table.push_back(UbenchDescriptor("rpc", ubench_rpc));
	return 0;
}

int 
Init(int debug_level, const char* xdst)
{
	Config::Init();
	Debug::Init(debug_level, NULL);

	global_ipc_layer = new client::Ipc(xdst);
	assert((global_ipc_layer->Init()) == 0);
	global_storage_system = new osd::client::StorageSystem(global_ipc_layer);
	assert((global_storage_system->Init()) == 0);
	assert(global_storage_system->Mount(poolpath, NULL, 0) == 0);
	return 0;
}


int
ShutDown()
{
	return 0;
}
