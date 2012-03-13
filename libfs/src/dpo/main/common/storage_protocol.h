#ifndef __STAMNOS_DPO_STORAGE_PROTOCOL_H
#define __STAMNOS_DPO_STORAGE_PROTOCOL_H

#include "ipc/ipc.h"
#include "dpo/main/common/obj.h"

namespace dpo {

class StorageProtocol {
public:
	
	enum RpcNumbers {
		kAllocateContainerVector = 40,
		kAllocateContainer
	};
	
	class ContainerRequest;

	class ContainerReply;
};


class StorageProtocol::ContainerRequest {
public:

	int type;
	int num;
};


class StorageProtocol::ContainerReply {
public:
	int                     index; // capability index
	::dpo::common::ObjectId oid;
}; 


} // namespace dpo



// Marshalling functions for user-defined classes.
// (these live in the global namespace to be visible from the rpc library

inline marshall& operator<<(marshall &m, ::dpo::StorageProtocol::ContainerRequest& req) {
	m << (unsigned int) req.type;
	m << (unsigned int) req.num;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::dpo::StorageProtocol::ContainerRequest& req) {
	u >> req.type;
	u >> req.num;
	return u;
}


inline marshall& operator<<(marshall &m, ::dpo::StorageProtocol::ContainerReply& val) {
	m << (unsigned int) val.index;
	m << val.oid;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::dpo::StorageProtocol::ContainerReply& val) {
	u >> val.index;
	u >> val.oid;
	return u;
}


#endif // __STAMNOS_DPO_STORAGE_PROTOCOL_H
