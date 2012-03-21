#ifndef __STAMNOS_FS_SERVER_SESSION_H
#define __STAMNOS_FS_SERVER_SESSION_H

#include "ssa/main/server/stsystem.h"
#include "ssa/main/server/ssa-opaque.h"
#include "ssa/main/server/session.h"
#include "bcs/main/server/bcs.h"

namespace server {

class Session: public ssa::server::SsaSession {
public:
	Session(Ipc* ipc, ssa::server::StorageSystemT<Session>* storage_system)
	{ 
		ipc_ = ipc;
		storage_system_ = storage_system;
	}

	int Init(int clt);

private:
	int               acl;
};


} // namespace server


#endif // __STAMNOS_FS_SERVER_SESSION_H
