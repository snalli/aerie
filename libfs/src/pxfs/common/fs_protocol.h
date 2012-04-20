#ifndef __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
#define __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H

#include "bcs/bcs.h"
#include "osd/main/common/stsystem.h"


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


namespace rpcfast {

inline marshall& operator<<(marshall &m, FileSystemProtocol::MountReply& val) {
	m << val.desc_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, FileSystemProtocol::MountReply& val) {
	u >> val.desc_;
	return u;
}

} // namespace rpcfast


namespace rpcnet {

inline marshall& operator<<(marshall &m, FileSystemProtocol::MountReply& val) {
	m << val.desc_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, FileSystemProtocol::MountReply& val) {
	u >> val.desc_;
	return u;
}

} // namespace rpcnet

#endif // __STAMNOS_PXFS_FILESYSTEM_PROTOCOL_H
