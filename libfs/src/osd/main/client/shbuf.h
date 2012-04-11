#ifndef __STAMNOS_OSD_CLIENT_SHARED_BUFFER_H
#define __STAMNOS_OSD_CLIENT_SHARED_BUFFER_H

#include "bcs/main/client/bcs-opaque.h"
#include "bcs/main/client/shbuf.h"

namespace osd {
namespace client {


class OsdSharedBuffer: public ::client::SharedBuffer {
public:
	OsdSharedBuffer(::client::Ipc* ipc, ::SharedBuffer::Descriptor& dsc)
		: SharedBuffer(ipc, dsc)
	{ }
};


} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_SHARED_BUFFER_H
