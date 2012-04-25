#ifndef __STAMNOS_RXFS_CLIENT_SUPERBLOCK_H
#define __STAMNOS_RXFS_CLIENT_SUPERBLOCK_H

#include <pthread.h>
#include "common/types.h"
#include "osd/containers/super/container.h"
#include "osd/main/common/obj.h"
#include "rxfs/client/dir_inode.h"

namespace client {

class SuperBlock {
public:
	static SuperBlock* Load(Session* session, osd::common::ObjectId oid, SuperBlock* sb) {
		osd::containers::client::SuperContainer::Object* obj = osd::containers::client::SuperContainer::Object::Load(oid);
		return new(sb) SuperBlock(obj);
	}
	
	SuperBlock()
	{ }
	
	SuperBlock(osd::containers::client::SuperContainer::Object* super_obj) 
		: super_obj_(super_obj)
	{ 
		root_ino_ = super_obj_->root(NULL).u64();
	}

	InodeNumber root_ino() { return root_ino_; }

private:
	osd::containers::client::SuperContainer::Object* super_obj_; 
	InodeNumber                                      root_ino_;
};

} // namespace client

#endif // __STAMNOS_RXFS_CLIENT_SUPERBLOCK_H
