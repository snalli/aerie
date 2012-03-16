#ifndef __STAMNOS_MFS_SERVER_FILESYSTEM_FACTORY_H
#define __STAMNOS_MFS_SERVER_FILESYSTEM_FACTORY_H

#include <string>
#include "server/fs_factory.h"
#include "server/session.h"
#include "dpo/main/server/dpo-opaque.h"
#include "dpo/main/common/obj.h"
#include "spa/pool/pool.h"
#include "common/const.h"

namespace mfs {
namespace server {

class FileSystemFactory: public ::server::FileSystemFactory {
public:
	FileSystemFactory();
	int Make(dpo::server::Dpo* dpo, size_t nblocks, size_t block_size, int flags);
	int Load(dpo::server::Dpo* dpo, int flags, ::server::FileSystem** filesystem);
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
