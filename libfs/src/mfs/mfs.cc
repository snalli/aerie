#include "client/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/sb.h"
#include "mfs/pstruct.h"

namespace mfs {

client::SuperBlock* CreateSuperBlock(client::ClientSession* session, void* ptr) {
	client::SuperBlock* sb;

	if (ptr) {
		// load superblock
		uint64_t sbu = reinterpret_cast<uint64_t>(ptr);
		PSuperBlock<client::ClientSession>* psb = PSuperBlock<client::ClientSession>::Load(sbu);
		if (psb) {
			sb = new SuperBlock(session, psb);
		} else {
			sb = NULL;
		}
	} else {
		// create file system superblock
		DirPnode<client::ClientSession>* dpnode = new(session) DirPnode<client::ClientSession>;
		PSuperBlock<client::ClientSession>* psb = new(session) PSuperBlock<client::ClientSession>;
		psb->root_ = reinterpret_cast<uint64_t>(dpnode);
		sb = new SuperBlock(session, psb);
	}
	return sb;
}

} // namespace mfs
