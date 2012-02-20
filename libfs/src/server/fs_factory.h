#ifndef __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H
#define __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H

#include "dpo/main/common/obj.h"

namespace server {

class Session;     // forward declaration
class FileSystem;  // forward declaration

class FileSystemFactory {
public:
	virtual int Make(Session* session, void* partition, dpo::common::ObjectId* oid) = 0;
	//virtual int Load(Session* session, dpo::common::ObjectId oid, SuperBlock** sbp) = 0;
	virtual int TypeID() = 0;
	virtual std::string TypeStr() = 0;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILESYSTEM_FACTORY_H
