#include "client/fsmgr.h"

namespace client {

FileSystemObjectManager::FileSystemObjectManager(rpcc* rpc_client, unsigned int principal_id)
{
	sb_factory_map_.set_empty_key(0);
}


void
FileSystemObjectManager::Register()
{
	
}



} // namespace client
