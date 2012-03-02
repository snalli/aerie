#include "server/session.h"
#include "dpo/main/server/dpo.h"
#include "dpo/main/server/session.h"
#include "common/errno.h"


namespace server {

int 
Session::Init(int clt, dpo::server::Dpo* dpo) 
{
	dpo::server::DpoSession::Init(clt, dpo);
	return E_SUCCESS;
}


} // namespace server
