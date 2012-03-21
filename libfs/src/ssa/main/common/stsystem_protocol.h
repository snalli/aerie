#ifndef __STAMNOS_SSA_STORAGE_SYSTEM_PROTOCOL_H
#define __STAMNOS_SSA_STORAGE_SYSTEM_PROTOCOL_H

#include "bcs/bcs.h"
#include "ssa/main/common/obj.h"
#include "bcs/main/common/shbuf_protocol.h"


class StorageSystemDescriptor {
public:
	StorageSystemDescriptor()
	{ }

	StorageSystemDescriptor(ssa::common::ObjectId oid, SharedBufferDescriptor shbuf_dsc)
		: oid_(oid),
		  shbuf_dsc_(shbuf_dsc)
	{ }
	
	ssa::common::ObjectId  oid_;
	SharedBufferDescriptor shbuf_dsc_;
};


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




class StorageSystemProtocol {
public:
	
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(SSA_STORAGESYSTEM_PROTOCOL)
	};

	class MountReply;
};


class StorageSystemProtocol::MountReply {
public:
	StorageSystemDescriptor desc_;
};


inline marshall& operator<<(marshall &m, StorageSystemProtocol::MountReply& val) {
	m << val.desc_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, StorageSystemProtocol::MountReply& val) {
	u >> val.desc_;
	return u;
}


#endif // __STAMNOS_SSA_STORAGE_SYSTEM_PROTOCOL_H
