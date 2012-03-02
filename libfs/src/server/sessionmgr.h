#ifndef __STAMNOS_PXFS_SERVER_SESSION_MANAGER_H
#define __STAMNOS_PXFS_SERVER_SESSION_MANAGER_H

#include "ipc/main/server/ipc-opaque.h"
#include "dpo/main/server/dpo-opaque.h"


namespace server {

class Session; // forward declaration

class SessionManager {
public:
	SessionManager(Ipc* ipc, dpo::server::Dpo* dpo)
		: ipc_(ipc),
		  dpo_(dpo)
	{ }
	
	int Init();
	int Create(int clt, Session** session);
	int Destroy();
	int Lookup(int clt, Session** session);

private:
	BaseSessionManager* base_session_mgr_;
	Ipc*                ipc_;
	dpo::server::Dpo*   dpo_;
};


} // namespace server




#endif // __STAMNOS_PXFS_SERVER_SESSION_MANAGER_H
