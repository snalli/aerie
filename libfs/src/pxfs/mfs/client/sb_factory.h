#ifndef __STAMNOS_MFS_CLIENT_SUPERBLOCK_FACTORY_H
#define __STAMNOS_MFS_CLIENT_SUPERBLOCK_FACTORY_H

#include <string>
#include "pxfs/client/sb_factory.h"
#include "pxfs/client/session.h"
#include "pxfs/common/const.h"

namespace mfs {
namespace client {

class SuperBlockFactory: public ::client::SuperBlockFactory {
public:
	SuperBlockFactory();
	int Make(::client::Session* session, ::client::SuperBlock** sbp);
	int Load(::client::Session* session, osd::common::ObjectId oid, ::client::SuperBlock** sbp);
	int TypeID() {
		return kMFS;
	}
	std::string TypeStr() {
		return "mfs";
	}
};

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_SUPERBLOCK_FACTORY_H
