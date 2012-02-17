#ifndef __STAMNOS_MFS_SERVER_FILESYSTEM_FACTORY_H
#define __STAMNOS_MFS_SERVER_FILESYSTEM_FACTORY_H

#include <string>
#include "server/fs_factory.h"
#include "server/session.h"
#include "dpo/main/common/obj.h"
#include "common/const.h"

namespace mfs {
namespace server {

class FileSystemFactory: public ::server::FileSystemFactory {
public:
	FileSystemFactory();
	int Make(::server::Session* session, dpo::common::ObjectId* oid);
	int TypeID() {
		return ::common::fs::kMFS;
	}
	std::string TypeStr() {
		return "mfs";
	}
};

} // namespace server
} // namespace mfs

#endif // __STAMNOS_MFS_SERVER_FILESYSTEM_FACTORY_H
