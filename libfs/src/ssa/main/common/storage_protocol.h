#ifndef __STAMNOS_SSA_STORAGE_PROTOCOL_H
#define __STAMNOS_SSA_STORAGE_PROTOCOL_H

#include "bcs/bcs.h"
#include "ssa/main/common/obj.h"

namespace ssa {

class StorageProtocol {
public:
	
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(SSA_STORAGE_PROTOCOL)
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
	::ssa::common::ObjectId oid;
}; 


} // namespace ssa



// Marshalling functions for user-defined classes.
// (these live in the global namespace to be visible from the rpc library

inline marshall& operator<<(marshall &m, ::ssa::StorageProtocol::ContainerRequest& req) {
	m << (unsigned int) req.type;
	m << (unsigned int) req.num;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::ssa::StorageProtocol::ContainerRequest& req) {
	u >> req.type;
	u >> req.num;
	return u;
}


inline marshall& operator<<(marshall &m, ::ssa::StorageProtocol::ContainerReply& val) {
	m << (unsigned int) val.index;
	m << val.oid;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::ssa::StorageProtocol::ContainerReply& val) {
	u >> val.index;
	u >> val.oid;
	return u;
}


#endif // __STAMNOS_SSA_STORAGE_PROTOCOL_H
