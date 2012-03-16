#ifndef __STAMNOS_FS_SERVER_SESSION_H
#define __STAMNOS_FS_SERVER_SESSION_H

#include "ssa/main/server/ssa.h"
#include "ssa/main/server/session.h"
#include "ipc/main/server/ipc.h"

namespace server {

class Session: public ssa::server::DpoSession {
public:
	Session(Ipc* ipc, ssa::server::Dpo* ssa)
	{ 
		ipc_ = ipc;
		ssa_ = ssa;
	}

	int Init(int clt);

private:
	int               acl;
};


} // namespace server


#endif // __STAMNOS_FS_SERVER_SESSION_H
