#include "ipc/main/server/cltdsc.h"

namespace server {

ClientDescriptor::ClientDescriptor(int clt, rpcc* rpc)
	: clt_(clt),
	  rpc_(rpc)
{ }


} // namespace server
