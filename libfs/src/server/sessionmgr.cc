#include "server/sessionmgr.h"
#include "server/session.h"
#include "common/debug.h"

namespace server {

int 
SessionManager::Init()
{
	return E_SUCCESS;
}


int
SessionManager::Create(int clt, Session** sessionp)
{
	int      ret;
	Session* session = new Session();

	dbg_log(DBG_INFO, "Create session for client %d\n", clt);

	if (!session) {
		return -E_NOMEM;
	}
	if ((ret = session->Init(clt, dpo_)) < 0) {
		return ret;
	}
	
	*sessionp = session;
	return E_SUCCESS;
}


} // namespace server
