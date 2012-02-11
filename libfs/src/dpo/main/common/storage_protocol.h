#ifndef __STAMNOS_DPO_STORAGE_PROTOCOL_H
#define __STAMNOS_DPO_STORAGE_PROTOCOL_H

#include "rpc/rpc.h"

namespace dpo {

class StorageProtocol {
public:
	
	enum RpcNumbers {
		kAllocateContainerVector = 40	
	};
	
	class ContainerRequest;

	class Capability;
};


class StorageProtocol::ContainerRequest {
public:

	int type;
	int num;
};


class StorageProtocol::Capability {
public:
	
private:
	int id;

}; 


} // namespace dpo



// Marshalling functions for user-defined classes.
// (these live in the global namespace to be visible from the rpc library

inline marshall& operator<<(marshall &m, ::dpo::StorageProtocol::ContainerRequest &req) {
	m << (unsigned int) req.type;
	m << (unsigned int) req.num;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::dpo::StorageProtocol::ContainerRequest &req) {
	u >> req.type;
	u >> req.num;
	return u;
}

#endif // __STAMNOS_DPO_STORAGE_PROTOCOL_H
