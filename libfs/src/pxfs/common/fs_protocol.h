#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "ipc/ipc.h"

class FileSystemProtocol {
public:
	
	enum RpcNumbers {
		kCreateFileSystem = 4500,
		kMountFileSystem	
	};
};


#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
