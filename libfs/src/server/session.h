#ifndef __STAMNOS_FS_SERVER_SESSION_H
#define __STAMNOS_FS_SERVER_SESSION_H

#include "dpo/main/server/dpo.h"
#include "dpo/main/server/session.h"
#include "ipc/main/server/ipc.h"

namespace server {

class Session: public dpo::server::DpoSession {
public:
	Session(Ipc* ipc, dpo::server::Dpo* dpo)
	{ 
		ipc_ = ipc;
		dpo_ = dpo;
	}

	int Init(int clt);

private:
	int               acl;
};


} // namespace server


#endif // __STAMNOS_FS_SERVER_SESSION_H
