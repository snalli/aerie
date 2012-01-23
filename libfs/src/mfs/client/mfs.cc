#include "client/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/client/sb_factory.h"
#include "mfs/client/inode_factory.h"
#include "mfs/const.h"

namespace mfs {
namespace client {

void 
RegisterBackend(::client::FileSystemObjectManager* fsomgr)
{
	SuperBlockFactory* sb_factory = new SuperBlockFactory();
	InodeFactory* inode_factory = new InodeFactory();
	fsomgr->Register(sb_factory, inode_factory);
	
	// register any file system specific container types with the main 
	// client object manager
	// DONE: no file system specific container types
}

} // namespace client
} // namespace mfs
