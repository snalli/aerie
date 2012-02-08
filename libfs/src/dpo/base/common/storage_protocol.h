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

};


class StorageProtocol::ContainerRequest {
public:

	int type;
	//int num;

	marshall& operator<<(marshall &m) {
		m << type;
		return m;
	}


};


} // namespace dpo



// Marshalling functions for user-defined classes.
// (these leave in the global namespace to be visible from rpc/

marshall& operator<<(marshall &m, ::dpo::StorageProtocol::ContainerRequest &req) {
	m << req.type;
	return m;
}


unmarshall& operator>>(unmarshall &u, ::dpo::StorageProtocol::ContainerRequest &req) {
	u >> req;
	return u;
}

#endif // __STAMNOS_DPO_STORAGE_PROTOCOL_H
