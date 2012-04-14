#ifndef __STAMNOS_CFS_SERVER_SUPERBLOCK_H
#define __STAMNOS_CFS_SERVER_SUPERBLOCK_H

#include <pthread.h>
#include "common/types.h"
#include "osd/containers/super/container.h"
#include "osd/main/common/obj.h"
#include "cfs/server/dir_inode.h"

namespace server {

class SuperBlock {
public:
	SuperBlock(osd::containers::server::SuperContainer::Object* super_obj) 
		: super_obj_(super_obj)
	{ 
		root_ino_ = super_obj_->root(NULL).u64();
	}

	InodeNumber root_ino() { return root_ino_; }

private:
	osd::containers::server::SuperContainer::Object* super_obj_; 
	InodeNumber                                      root_ino_;
};

} // namespace server

#endif // __STAMNOS_CFS_SERVER_SUPERBLOCK_H
