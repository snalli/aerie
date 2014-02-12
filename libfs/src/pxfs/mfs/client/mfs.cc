#define  __CACHE_GUARD__

#include "pxfs/mfs/client/sb_factory.h"
#include "pxfs/mfs/client/inode_factory.h"

#include "pxfs/client/backend.h"

namespace mfs {
namespace client {

void 
RegisterBackend(::client::FileSystemObjectManager* fsomgr)
{
	SuperBlockFactory* sb_factory = new SuperBlockFactory();
	InodeFactory*      inode_factory = new InodeFactory();
	fsomgr->Register(sb_factory, inode_factory);
	
	// register any file system specific container types with the
	// client osd object manager
	// DONE: no file system specific container types
}

} // namespace client
} // namespace mfs
