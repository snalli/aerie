#ifndef __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H
#define __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H

#include "dpo/main/common/obj.h"
#include "dpo/main/server/dpo-opaque.h"
#include "sal/pool/pool.h"

namespace server {

class Session;     // forward declaration
class FileSystem;  // forward declaration

class FileSystemFactory {
public:
	virtual int Make(StoragePool* pool, size_t nblocks, size_t block_size, int flags) = 0;
	virtual int Load(StoragePool* pool, int flags, FileSystem** filesystem) = 0;
	virtual int TypeID() = 0;
	virtual std::string TypeStr() = 0;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H
