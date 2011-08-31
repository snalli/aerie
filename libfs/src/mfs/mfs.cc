#include "client/backend.h"
#include "mfs/mfs_i.h"
#include "mfs/sb.h"
#include "mfs/pstruct.h"


namespace mfs {

client::SuperBlock* CreateSuperBlock(void* ptr) {
	client::SuperBlock* sb;

	if (ptr) {
		// load superblock
		uint64_t sbu = reinterpret_cast<uint64_t>(ptr);
		PSuperBlock* psb = PSuperBlock::Load(sbu);
		if (psb) {
			sb = new SuperBlock(psb);
		} else {
			sb = NULL;
		}
	} else {
		// create file system superblock
		DirPnode* dpnode = new(client::global_smgr) DirPnode;
		PSuperBlock* psb = new(client::global_smgr) PSuperBlock;
		psb->root_ = reinterpret_cast<uint64_t>(dpnode);
		sb = new SuperBlock(psb);
	}
	return sb;
}

} // namespace mfs
