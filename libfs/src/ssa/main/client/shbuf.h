#ifndef __STAMNOS_SSA_CLIENT_SHARED_BUFFER_H
#define __STAMNOS_SSA_CLIENT_SHARED_BUFFER_H

#include "bcs/main/client/bcs-opaque.h"
#include "bcs/main/client/shbuf.h"

namespace ssa {
namespace client {


class SsaSharedBuffer: public ::client::SharedBuffer {
public:
	SsaSharedBuffer(::client::Ipc* ipc, ::SharedBuffer::Descriptor& dsc)
		: SharedBuffer(ipc, dsc)
	{ }
};


} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_SHARED_BUFFER_H
