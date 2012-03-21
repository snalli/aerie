#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "bcs/bcs.h"
#include "ssa/main/common/stsystem_protocol.h"


class FileSystemProtocol {
public:
	
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(PXFS_FILESYSTEM_PROTOCOL)
	};

	class MountReply;
};


class FileSystemProtocol::MountReply {
public:
	StorageSystemDescriptor desc_;
};


inline marshall& operator<<(marshall &m, FileSystemProtocol::MountReply& val) {
	m << val.desc_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, FileSystemProtocol::MountReply& val) {
	u >> val.desc_;
	return u;
}


#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
