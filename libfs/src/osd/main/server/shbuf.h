#ifndef __STAMNOS_OSD_SERVER_SHARED_BUFFER_H
#define __STAMNOS_OSD_SERVER_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/server/bcs-opaque.h"
#include "bcs/main/server/shbuf.h"

namespace osd {
namespace server {

class OsdSharedBuffer: public ::server::SharedBuffer {
public:
	static SharedBuffer* Make() {
		return new OsdSharedBuffer();
	}

	int Consume(::server::BcsSession* session);
};


} // namespace server
} // namespace osd

#endif // __STAMNOS_OSD_SERVER_SHARED_BUFFER_H
