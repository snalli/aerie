#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "ipc/ipc.h"

class FileSystemProtocol {
public:
	
	enum RpcNumbers {
		kCreate = 4500,
		kMount	
	};
};


#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
