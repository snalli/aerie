#ifndef __STAMNOS_SSA_SERVER_PUBLISHER_H
#define __STAMNOS_SSA_SERVER_PUBLISHER_H

#include "bcs/bcs.h"
#include "ssa/main/server/ssa-opaque.h"
#include "ssa/main/common/publisher.h"

namespace ssa {
namespace server {

static const int kLogicalOpMaxCount = 1024;  // CONFIGURATION CONSTANT

// header_buf points to a buffer that holds the logical operation header. 
// the buffer must have enough space to hold the rest of the logical operation
// when decoded
typedef int (*LogicalOperation)(SsaSession* session, char* lgc_op_hdr, ssa::Publisher::Messages::BaseMessage* next);

class Publisher {
public:
	Publisher(::server::Ipc* ipc);

	int Init();
	int Publish(SsaSession* session);
	int RegisterOperation(int lgc_op_id, LogicalOperation lgc_op);

	class IpcHandlers {
	public:
		int Register(Publisher* publisher);

		int Publish(int clt, int& r);

	private:
		Publisher* publisher_;
	}; 

private:
	::server::Ipc*          ipc_;
	Publisher::IpcHandlers  ipc_handlers_;
	LogicalOperation        lgc_op_array_[kLogicalOpMaxCount];   
};

} // namespace server
} // namespace ssa


#endif // __STAMNOS_SSA_SERVER_PUBLISHER_H
