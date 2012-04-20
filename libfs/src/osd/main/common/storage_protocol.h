#ifndef __STAMNOS_OSD_STORAGE_PROTOCOL_H
#define __STAMNOS_OSD_STORAGE_PROTOCOL_H

#include "bcs/bcs.h"
#include "osd/main/common/obj.h"

namespace osd {


class StorageProtocol {
public:
	
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(OSD_STORAGE_PROTOCOL)
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
	::osd::common::ObjectId oid;
}; 


} // namespace osd



// Marshalling functions for user-defined classes.
// (these live in the global namespace to be visible from the rpc library

namespace rpcnet {

inline marshall& operator<<(marshall &m, ::osd::StorageProtocol::ContainerRequest& req) {
	m << (unsigned int) req.type;
	m << (unsigned int) req.num;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::osd::StorageProtocol::ContainerRequest& req) {
	u >> req.type;
	u >> req.num;
	return u;
}


inline marshall& operator<<(marshall &m, ::osd::StorageProtocol::ContainerReply& val) {
	m << (unsigned int) val.index;
	m << val.oid;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::osd::StorageProtocol::ContainerReply& val) {
	u >> val.index;
	u >> val.oid;
	return u;
}

} // namespace rpcnet


namespace rpcfast {

inline marshall& operator<<(marshall &m, ::osd::StorageProtocol::ContainerRequest& req) {
	m << (unsigned int) req.type;
	m << (unsigned int) req.num;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::osd::StorageProtocol::ContainerRequest& req) {
	u >> req.type;
	u >> req.num;
	return u;
}


inline marshall& operator<<(marshall &m, ::osd::StorageProtocol::ContainerReply& val) {
	m << (unsigned int) val.index;
	m << val.oid;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, ::osd::StorageProtocol::ContainerReply& val) {
	u >> val.index;
	u >> val.oid;
	return u;
}

} // namespace rpcfast


#endif // __STAMNOS_OSD_STORAGE_PROTOCOL_H
