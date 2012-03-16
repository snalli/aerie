#ifndef __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H
#define __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H

#include "ssa/main/common/obj.h"
#include "ssa/main/server/ssa-opaque.h"
#include "spa/pool/pool.h"

namespace server {

class Session;     // forward declaration
class FileSystem;  // forward declaration

class FileSystemFactory {
public:
	virtual int Make(ssa::server::Dpo* ssa, size_t nblocks, size_t block_size, int flags) = 0;
	virtual int Load(ssa::server::Dpo* ssa, int flags, FileSystem** filesystem) = 0;
	virtual int TypeID() = 0;
	virtual std::string TypeStr() = 0;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H
