#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "bcs/bcs.h"

class FileSystemProtocol {
public:
	
	enum RpcNumbers {
		PXFS_FILESYSTEM_PROTOCOL(RPC_NUMBER)
	};
};


#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
