#ifndef __STAMNOS_MFS_CLIENT_SUPERBLOCK_FACTORY_H
#define __STAMNOS_MFS_CLIENT_SUPERBLOCK_FACTORY_H

#include <string>
#include "client/sb_factory.h"
#include "client/session.h"
#include "common/const.h"

namespace mfs {
namespace client {

class SuperBlockFactory: public ::client::SuperBlockFactory {
public:
	SuperBlockFactory();
	int Make(::client::Session* session, ::client::SuperBlock** sbp);
	int Load(::client::Session* session, dpo::common::ObjectId oid, ::client::SuperBlock** sbp);
	int TypeID() {
		return ::common::fs::kMFS;
	}
	std::string TypeStr() {
		return "mfs";
	}
};

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_SUPERBLOCK_FACTORY_H
