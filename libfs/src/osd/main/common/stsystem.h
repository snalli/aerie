#ifndef __STAMNOS_OSD_STORAGE_SYSTEM_PROTOCOL_H
#define __STAMNOS_OSD_STORAGE_SYSTEM_PROTOCOL_H

#include "bcs/bcs.h"
#include "osd/main/common/obj.h"
#include "bcs/main/common/shbuf.h"


class StorageSystemDescriptor {
public:
	StorageSystemDescriptor()
	{ }

	StorageSystemDescriptor(osd::common::ObjectId oid, SharedBuffer::Descriptor shbuf_dsc)
		: oid_(oid),
		  shbuf_dsc_(shbuf_dsc)
	{ }
	
	osd::common::ObjectId  oid_;
	SharedBuffer::Descriptor shbuf_dsc_;
};


class StorageSystemProtocol {
public:
	
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(OSD_STORAGESYSTEM_PROTOCOL)
	};

	class MountReply;
};


class StorageSystemProtocol::MountReply {
public:
	StorageSystemDescriptor desc_;
};


namespace rpcnet {

inline marshall& operator<<(marshall &m, StorageSystemDescriptor& val) {
	m << val.oid_;
    m << val.shbuf_dsc_;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, StorageSystemDescriptor& val) {
	u >> val.oid_;
    u >> val.shbuf_dsc_;
	return u;
}


inline marshall& operator<<(marshall &m, StorageSystemProtocol::MountReply& val) {
	m << val.desc_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, StorageSystemProtocol::MountReply& val) {
	u >> val.desc_;
	return u;
}

} // namespace rpcnet


namespace rpcfast {

inline marshall& operator<<(marshall &m, StorageSystemDescriptor& val) {
	m << val.oid_;
    m << val.shbuf_dsc_;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, StorageSystemDescriptor& val) {
	u >> val.oid_;
    u >> val.shbuf_dsc_;
	return u;
}


inline marshall& operator<<(marshall &m, StorageSystemProtocol::MountReply& val) {
	m << val.desc_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, StorageSystemProtocol::MountReply& val) {
	u >> val.desc_;
	return u;
}

} // namespace rpcfast


#endif // __STAMNOS_OSD_STORAGE_SYSTEM_PROTOCOL_H
