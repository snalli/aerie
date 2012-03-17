#ifndef __STAMNOS_SSA_SERVER_SESSION_MANAGER_H
#define __STAMNOS_SSA_SERVER_SESSION_MANAGER_H

#include "ipc/main/server/ipc.h"
#include "ssa/main/server/ssa-opaque.h"
#include "common/debug.h"
#include "common/errno.h"

namespace server {

template<typename Session>
class SessionManager {
public:
	SessionManager(Ipc* ipc, ssa::server::StorageSystem* storage_system)
		: ipc_(ipc),
		  storage_system_(storage_system)
	{ }
	
	int Init();
	int Create(int clt, Session** session);
	int Destroy();
	int Lookup(int clt, Session** session);

private:
	BaseSessionManager*          base_session_mgr_;
	Ipc*                         ipc_;
	ssa::server::StorageSystem*  storage_system_;
};

template<typename Session>
int 
SessionManager<Session>::Init()
{
	base_session_mgr_ = ipc_->session_manager();
	return E_SUCCESS;
}


template<typename Session>
int
SessionManager<Session>::Create(int clt, Session** sessionp)
{
	int      ret;
	Session* session = new Session(ipc_, storage_system_);

	dbg_log(DBG_INFO, "Create session for client %d\n", clt);

	if (!session) {
		return -E_NOMEM;
	}
	if ((ret = session->Init(clt)) < 0) {
		return ret;
	}
	if ((ret = base_session_mgr_->Insert(clt, session)) < 0) {
		return ret;
	}

	*sessionp = session;
	return E_SUCCESS;
}


} // namespace server


#endif // __STAMNOS_SSA_SERVER_SESSION_MANAGER_H
