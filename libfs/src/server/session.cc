#include "server/session.h"
#include "ssa/main/server/ssa.h"
#include "ssa/main/server/session.h"
#include "common/errno.h"


namespace server {

int 
Session::Init(int clt) 
{
	ssa::server::DpoSession::Init(clt);
	return E_SUCCESS;
}


} // namespace server
