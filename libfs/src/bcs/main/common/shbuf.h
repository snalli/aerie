#ifndef __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	
#define __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	

#include <string>
#include <stdint.h>
#include "bcs/backend/rpc.h"
#include "bcs/rpcnum.h"
#include "common/errno.h"


class SharedBuffer {
public:
	class Protocol;   // RPC protocol
	class Descriptor; // Descriptor exchanged between client/server identifying a shared buffer
	class Header;     // Header kept along with the buffer
};


class SharedBuffer::Protocol {
public:
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(BCS_SHARED_BUFFER_PROTOCOL)
	};
};

class SharedBuffer::Descriptor {
public:
	Descriptor()
	{ }

	Descriptor(int id, std::string path, size_t size)
		: id_(id),
		  path_(path),
		  size_(size)
	{ }

	int          id_;   // capability: identifier private to a client
	std::string  path_;
	unsigned int size_;
};

inline marshall& operator<<(marshall &m, SharedBuffer::Descriptor& val) {
    m << val.path_;
    m << val.id_;
    m << val.size_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, SharedBuffer::Descriptor& val) {
    u >> val.path_;
    u >> val.id_;
    u >> val.size_;
    return u;
}


class SharedBuffer::Header {
public:
	uint64_t start_; // updated by the server (consumer)
	uint64_t end_;   // updated by the client (producer)
};


#endif // __STAMNOS_BCS_COMMON_SHARED_BUFFER_H	
