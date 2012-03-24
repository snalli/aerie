#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "ssa/main/client/ssa-opaque.h"
#include "ssa/main/client/session.h"

namespace client {

class Session: public ssa::client::SsaSession {
public:
	Session(ssa::client::StorageSystem* stsystem)
		: SsaSession(stsystem)
	{ }
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
