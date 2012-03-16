#include "server/sessionmgr.h"
#include "server/session.h"
#include "common/debug.h"

namespace server {

int 
SessionManager::Init()
{
	base_session_mgr_ = ipc_->session_manager();
	return E_SUCCESS;
}


int
SessionManager::Create(int clt, Session** sessionp)
{
	int      ret;
	Session* session = new Session(ipc_, ssa_);

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
