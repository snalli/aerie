#include "server/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/server/fs_factory.h"
#include "mfs/const.h"

namespace mfs {
namespace server {

void 
RegisterBackend(::server::FileSystemManager* fsmgr)
{
	FileSystemFactory* fs_factory = new FileSystemFactory();
	fsmgr->Register(fs_factory);
	
	// register any file system specific container types with the
	// client dpo object manager
	// DONE: no file system specific container types
}

} // namespace client
} // namespace mfs
