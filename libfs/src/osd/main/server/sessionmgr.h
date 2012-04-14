#ifndef __STAMNOS_OSD_SERVER_SESSION_MANAGER_H
#define __STAMNOS_OSD_SERVER_SESSION_MANAGER_H

#include "bcs/main/server/bcs.h"
#include "osd/main/server/osd-opaque.h"
#include "bcs/main/common/cdebug.h"
#include "common/errno.h"

namespace osd {
namespace server {

template<typename Session, typename StorageSystem>
class SessionManager {
public:
	SessionManager(::server::Ipc* ipc, StorageSystem* storage_system)
		: ipc_(ipc),
		  storage_system_(storage_system)
	{ }
	
	int Init();
	int Create(int clt, Session** session);
	int Destroy();
	int Lookup(int clt, Session** session);

private:
	::server::BaseSessionManager* base_session_mgr_;
	::server::Ipc*                ipc_;
	StorageSystem*                storage_system_;
};


template<typename Session, typename StorageSystem>
int 
SessionManager<Session, StorageSystem>::Init()
{
	base_session_mgr_ = ipc_->session_manager();
	return E_SUCCESS;
}


template<typename Session, typename StorageSystem>
int
SessionManager<Session, StorageSystem>::Create(int clt, Session** sessionp)
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
} // namespace osd

#endif // __STAMNOS_OSD_SERVER_SESSION_MANAGER_H
