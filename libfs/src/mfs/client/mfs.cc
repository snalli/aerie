#include "client/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/client/sb_factory.h"
#include "mfs/client/sb.h"
#include "mfs/client/inode_factory.h"
#include "mfs/client/inode.h"
#include "mfs/psb.h"

namespace mfs {
namespace client {

void 
RegisterBackend(client::FileSystemObjectManager* fsomgr)
{
	SuperBlockFactory* sb_factory = new SuperBlockFactory();
	fsomgr->Register(sb_factory, inode_factory)
}


// FIXME: part of this code file should re-structured into an inode factory

//FIXME: SUPERBLOCK
#if 0
client::SuperBlock* CreateSuperBlock(client::Session* session, void* ptr) {
	client::SuperBlock* sb;

	if (ptr) {
		// load superblock
		uint64_t sbu = reinterpret_cast<uint64_t>(ptr);
		PSuperBlock<client::Session>* psb = PSuperBlock<client::Session>::Load(sbu);
		if (psb) {
			sb = new SuperBlock(session, psb);
		} else {
			sb = NULL;
		}
	} else {
		// create file system superblock
		DirPnode<client::Session>* dpnode = new(session) DirPnode<client::Session>;
		PSuperBlock<client::Session>* psb = new(session) PSuperBlock<client::Session>;
		psb->root_ = reinterpret_cast<uint64_t>(dpnode);
		sb = new SuperBlock(session, psb);
	}
	return sb;
}

#endif

} // namespace client
} // namespace mfs
