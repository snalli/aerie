#ifndef __STAMNOS_OSD_SERVER_PUBLISHER_H
#define __STAMNOS_OSD_SERVER_PUBLISHER_H

#include "bcs/bcs.h"
#include "osd/main/server/osd-opaque.h"
#include "osd/main/common/publisher.h"

namespace osd {
namespace server {

static const int kLogicalOpMaxCount = 1024;  // CONFIGURATION CONSTANT

// header_buf points to a buffer that holds the logical operation header. 
// the buffer must have enough space to hold the rest of the logical operation
// when decoded
typedef int (*LogicalOperation)(OsdSession* session, char* lgc_op_hdr, osd::Publisher::Message::BaseMessage* next);

class Publisher {
public:
	Publisher(::server::Ipc* ipc);

	int Init();
	int Publish(OsdSession* session);
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
} // namespace osd


#endif // __STAMNOS_OSD_SERVER_PUBLISHER_H
