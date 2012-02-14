#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "ipc/ipc.h"

class FilesystemProtocol {
public:
	
	enum RpcNumbers {
		kCreateFilesystem = 5000	
	};
};


#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
