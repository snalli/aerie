#ifndef __STAMNOS_SSA_SERVER_SHARED_BUFFER_H
#define __STAMNOS_SSA_SERVER_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/server/bcs-opaque.h"
#include "bcs/main/server/shbuf.h"

namespace ssa {
namespace server {

class SsaSharedBuffer: public ::server::SharedBuffer {
public:
	static SharedBuffer* Make() {
		return new SsaSharedBuffer();
	}

	int Consume(::server::BcsSession* session);
};


} // namespace server
} // namespace ssa

#endif // __STAMNOS_SSA_SERVER_SHARED_BUFFER_H
