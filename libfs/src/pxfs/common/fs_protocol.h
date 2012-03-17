#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "ipc/ipc.h"

class FileSystemProtocol {
public:
	
	enum RpcNumbers {
		kMount = 4500
	};
};


#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
